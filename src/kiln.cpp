#include "skopin/kiln.hpp"

#include <stdexcept>

namespace skopin {

Kiln::Kiln() {
}

Cone Kiln::cone() const {
    return cone_;
}

Door Kiln::door() const {
    return door_;
}

int Kiln::stokes() const {
    return stokes_;
}

std::vector<Ware> Kiln::stack() const {
    return stack_;
}

void Kiln::open() {
    if (door_ == Door::Open || cone_ == Cone::Glaze) {
        throw KilnError("Cannot open door");
    }

    door_ = Door::Open;
}

void Kiln::close() {
    if (door_ == Door::Closed) {
        throw KilnError("Door already closed");
    }

    door_ = Door::Closed;
}


void Kiln::fire() {
    if (door_ == Door::Open) {
        throw KilnError("Door is open");
    }

    if (cone_ == Cone::Glaze) {
        throw KilnError("Maximum temperature");
    }

    for (const Ware& ware : stack_) {
        if (ware.thickness_mm > kCrackThicknessMm) {
            throw KilnError("Ware is too thick");
        }

        if (cone_ == Cone::Bisque &&
            ware.program == Program::Bisque) {
            throw KilnError("Bisque cannot reach glaze");
        }
    }

    if (cone_ == Cone::Cold) {
        cone_ = Cone::Bisque;
    } else {
        cone_ = Cone::Glaze;
        glazeFired_ = true;
    }

    ++stokes_;
}



void Kiln::cool() {
    if (door_ == Door::Open) {
        throw KilnError("Door is open");
    }

    if (cone_ == Cone::Cold) {
        throw KilnError("Already cold");
    }

    if (cone_ == Cone::Bisque) {
        for (const Ware& ware : stack_) {
            if (ware.program == Program::Glaze && !glazeFired_) {
                throw KilnError("Glaze not fired");
            }
        }
        cone_ = Cone::Cold;
    } else {
        cone_ = Cone::Bisque;
    }

    ++stokes_;
}



void Kiln::load(const Ware& ware) {
    if (door_ != Door::Open) {
        throw KilnError("Door must be open");
    }

    if (cone_ == Cone::Glaze ||
        (cone_ == Cone::Bisque && ware.program != Program::Glaze)) {
        throw KilnError("Wrong temperature for loading");
    }

    if (validateWare(ware).has_value()) {
        throw KilnError("Invalid ware");
    }

    for (const Ware& existing : stack_) {
        if (existing.mark == ware.mark) {
            throw KilnError("Duplicate mark");
        }

        if (existing.kind == WareKind::Sagger) {
            throw KilnError("Sagger occupies kiln");
        }
    }

    if (ware.kind == WareKind::Sagger && !stack_.empty()) {
        throw KilnError("Sagger must be alone");
    }

    if (!fitsInKiln(stack_, ware)) {
        throw KilnError("Not enough space");
    }

    stack_.push_back(ware);

    if (ware.program == Program::Glaze) {
        glazeFired_ = false;
    }
}



void Kiln::unload(const std::string& mark) {
    if (door_ != Door::Open) {
        throw KilnError("Door must be open");
    }

    if (stack_.empty()) {
        throw KilnError("Kiln is empty");
    }

    const Ware& top = stack_.back();

    if (top.mark != mark) {
        throw KilnError("Wrong mark");
    }

    if (top.program == Program::Glaze && cone_ != Cone::Cold) {
        throw KilnError("Glaze must be unloaded cold");
    }

    stack_.pop_back();
}


}  // namespace skopin
