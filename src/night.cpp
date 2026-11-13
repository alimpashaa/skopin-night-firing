#include "skopin/night.hpp"
#include "skopin/kiln.hpp"
#include "skopin/measures.hpp"

#include <unordered_set>
#include <vector>

namespace skopin {

NightReport runNightFire(const std::vector<Ware>& queue) {
    NightReport report;
    std::unordered_set<std::string> seen;
    std::vector<Ware> waiting;

    // Проверяем очередь
    for (const Ware& ware : queue) {
        if (!seen.insert(ware.mark).second) {
            report.refused.push_back({ware.mark, "duplicate"});
        } else if (validateWare(ware).has_value()) {
            report.refused.push_back({ware.mark, "invalid"});
        } else if (ware.thickness_mm > kCrackThicknessMm) {
            report.refused.push_back({ware.mark, "thick"});
        } else {
            waiting.push_back(ware);
        }
    }

    // Формируем партии для обжига
    std::vector<std::vector<Ware>> batches;

    while (!waiting.empty()) {
        std::vector<Ware> batch;
        std::vector<bool> used(waiting.size(), false);

        batch.push_back(waiting[0]);
        used[0] = true;

        Program program = waiting[0].program;
        bool skipped = false;

        if (waiting[0].kind != WareKind::Sagger) {
            for (size_t i = 1; i < waiting.size(); ++i) {
                const Ware& ware = waiting[i];

                if (ware.program != program ||
                    ware.kind == WareKind::Sagger) {
                    break;
                }

                if (fitsInKiln(batch, ware) &&
                    (!skipped || ware.kind == WareKind::Rush)) {
                    batch.push_back(ware);
                    used[i] = true;
                } else {
                    skipped = true;
                }
            }
        }

        batches.push_back(batch);

        std::vector<Ware> remaining;
        for (size_t i = 0; i < waiting.size(); ++i) {
            if (!used[i]) {
                remaining.push_back(waiting[i]);
            }
        }
        waiting = remaining;
    }

    // Обжигаем партии
    Kiln kiln;

    for (size_t i = 0; i < batches.size(); ++i) {
        const auto& batch = batches[i];
        Program program = batch[0].program;

        if (program == Program::Bisque) {
            if (kiln.door() == Door::Open) {
                kiln.close();
            }

            if (kiln.cone() == Cone::Bisque) {
                kiln.cool();
            }

            kiln.open();

            for (size_t j = batch.size(); j > 0; --j) {
                kiln.load(batch[j - 1]);
            }

            kiln.close();
            kiln.fire();
            kiln.open();

            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            bool nextGlaze =
                i + 1 < batches.size() &&
                batches[i + 1][0].program == Program::Glaze;

            if (!nextGlaze) {
                kiln.close();
                kiln.cool();
            }
        } else {
            if (kiln.door() == Door::Closed) {
                kiln.open();
            }

            for (size_t j = batch.size(); j > 0; --j) {
                kiln.load(batch[j - 1]);
            }

            kiln.close();

            if (kiln.cone() == Cone::Cold) {
                kiln.fire();
            }

            kiln.fire();
            kiln.cool();
            kiln.cool();

            kiln.open();

            for (const Ware& ware : batch) {
                kiln.unload(ware.mark);
                report.passed.push_back(ware.mark);
            }

            kiln.close();
        }
    }

    report.stokes = kiln.stokes();
    return report;
}
} // namespace skopin
