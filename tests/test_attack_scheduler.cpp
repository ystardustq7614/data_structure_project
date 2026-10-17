// 直接连接产品配置加载器与调度器，保留历史产品轨迹作为结构重构的行为基线。
#include "../26C-/core/attackplanloader.h"
#include "../26C-/core/attackscheduler.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>
#include <type_traits>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

QString readText(const QString& path)
{
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "could not read trace fixture");
    return QString::fromUtf8(file.readAll()).replace("\r\n", "\n").trimmed();
}

void writeText(const QString& path, const QByteArray& text)
{
    QFile file(path);
    require(file.open(QIODevice::WriteOnly), "could not create rhythm override");
    require(file.write(text) == text.size(), "could not write rhythm override");
}

} // namespace

int main(int argc, char** argv)
{
    static_assert(!std::is_default_constructible_v<AttackPlan>,
                  "an unvalidated empty attack plan must not be publicly constructible");
    QCoreApplication app(argc, argv);
    AttackScheduler scheduler;
    require(!scheduler.isReady() && scheduler.currentTick() == 0
                && scheduler.totalEnemies() == 0 && scheduler.preview().isEmpty()
                && scheduler.advance(0).isEmpty(),
            "uninitialized scheduler accepted a game tick");
    QTemporaryDir directory;
    require(directory.isValid(), "temporary directory failed");
    const QString builtinPath = QStringLiteral(":/config/attack_waves.json");
    const QString overridePath = directory.filePath("attack_rhythm.json");
    auto loadAndReset = [&](const QString& rhythmPath) {
        AttackPlanLoadResult result = AttackPlanLoader::load(builtinPath, rhythmPath);
        if (result.plan) scheduler.reset(*result.plan);
        return result;
    };
    AttackPlanLoadResult loaded = loadAndReset(overridePath);
    require(loaded.plan.has_value(), "default schedule did not load");
    require(loaded.warning.isEmpty() && loaded.error.isEmpty(),
            "missing override was not silent");
    require(scheduler.totalEnemies() == 90, "default roster is not 90 enemies");

    // 配置可共享，事件堆和运行计数不能共享；重置其中一局不影响另一局。
    const AttackPlan builtinPlan = *loaded.plan;
    AttackScheduler independent;
    independent.reset(builtinPlan);
    scheduler.advance(0);
    require(independent.currentTick() == 0 && independent.spawnedCount() == 0
                && independent.preview().first().dueTick == 23,
            "advancing one scheduler changed another using the same plan");
    independent.advance(0);
    scheduler.reset(builtinPlan);
    require(independent.currentTick() == 1 && scheduler.currentTick() == 0,
            "resetting one scheduler changed another using the same plan");

    const auto firstPreview = scheduler.preview(3);
    require(firstPreview.size() == 3 && firstPreview.at(0).dueTick == 23,
            "first preview does not show the first spawn");
    require(scheduler.preview(3).at(0).sequence == firstPreview.at(0).sequence,
            "preview modified the heap");

    QStringList trace;
    qint64 waveStart = 0;
    int generated = 0;
    int reinforcements = 0;
    int bossPosition = 0;
    for (int frame = 0; frame < 3000 && scheduler.hasPendingGeneration(); ++frame) {
        for (const AttackScheduler::Event& event : scheduler.advance(0)) {
            if (event.kind == AttackScheduler::Kind::WaveStart) {
                waveStart = scheduler.currentTick();
                trace << QString("WAVE %1 %2")
                             .arg(event.waveIndex + 1).arg(scheduler.currentTick());
            } else {
                ++generated;
                if (event.kind == AttackScheduler::Kind::Reinforcement) ++reinforcements;
                if (event.enemyType == "boss") bossPosition = generated;
                trace << QString("SPAWN %1 %2 %3")
                             .arg(event.waveIndex + 1)
                             .arg(scheduler.currentTick() - waveStart)
                             .arg(event.enemyType);
            }
        }
    }
    require(generated == 90 && scheduler.spawnedCount() == 90,
            "scheduler did not dispatch all 90 enemies");
    require(reinforcements == 18 && bossPosition == 72,
            "reinforcement labels or boss position changed");
    const QString fixture = readText(QDir(QCoreApplication::applicationDirPath())
                                         .filePath("legacy_spawn_trace.txt"));
    require(trace.join('\n') == fixture,
            "scheduler differs from the pre-change product spawn trace");

    scheduler.reset(builtinPlan);
    for (int tick = 0; tick < 121; ++tick) scheduler.advance(0);
    require(scheduler.spawnedCount() == 8 && scheduler.waitingForClear(),
            "wave one did not finish at tick 121");
    for (int tick = 0; tick < 25; ++tick) scheduler.advance(7);
    require(scheduler.currentWaveIndex() == 0 && scheduler.preview().isEmpty(),
            "next wave was scheduled before clear threshold");
    const qint64 firstClearTick = scheduler.currentTick() + 1;
    scheduler.advance(6);
    require(scheduler.preview().size() == 1
                && scheduler.preview().first().dueTick == firstClearTick + 19,
            "twenty-check wave wait has an off-by-one error");
    for (int tick = 0; tick < 18; ++tick) scheduler.advance(6);
    require(scheduler.currentWaveIndex() == 0, "wave began before twentieth check");
    const auto start = scheduler.advance(6);
    require(start.size() == 1 && start.first().kind == AttackScheduler::Kind::WaveStart
                && scheduler.currentWaveIndex() == 1,
            "wave did not begin on twentieth check");

    writeText(overridePath,
              R"({"version":1,"waves":[{"openingDelayFrames":5},{},{},{},{},{}]})");
    loaded = loadAndReset(overridePath);
    require(loaded.plan.has_value() && loaded.warning.isEmpty() && loaded.error.isEmpty(),
            "valid rhythm override was rejected");
    require(scheduler.preview().first().dueTick == 5,
            "valid rhythm override did not change first spawn");
    require(builtinPlan.waves().first().openingDelayFrames == 23
                && independent.currentTick() == 1
                && independent.preview().first().dueTick == 23,
            "loading a new rhythm changed an existing plan or another scheduler");

    writeText(overridePath,
              R"({"version":1,"waves":[{"openingDelayFrames":5},{"burstGapFrames":1.5},{},{},{},{}]})");
    loaded = loadAndReset(overridePath);
    require(loaded.plan.has_value() && !loaded.warning.isEmpty() && loaded.error.isEmpty(),
            "fractional rhythm override was not rejected");
    require(scheduler.preview().first().dueTick == 23,
            "invalid override partly changed the built-in schedule");

    writeText(overridePath,
              R"({"version":1,"waves":[{"groups":[]},{},{},{},{},{}]})");
    loaded = loadAndReset(overridePath);
    require(loaded.plan.has_value() && !loaded.warning.isEmpty(),
            "external roster replacement was accepted");

    const QByteArray invalidOverrides[] = {
        "{",
        R"({"version":1,"waves":[{"openingDelayFrames":0},{},{},{},{},{}]})",
        R"({"version":1,"waves":[{"openingDelayFrames":-1},{},{},{},{},{}]})",
        R"({"version":1,"waves":[{"openingDelayFrames":10001},{},{},{},{},{}]})",
        R"({"version":1,"waves":[{"burstSize":2,"openingBurstSize":3},{},{},{},{},{}]})",
        R"({"version":1,"waves":[{"enemy":"boss"},{},{},{},{},{}]})",
        R"({"version":1,"waves":[{},{},{},{},{}]})"
    };
    for (const QByteArray& invalid : invalidOverrides) {
        writeText(overridePath, invalid);
        loaded = loadAndReset(overridePath);
        require(loaded.plan.has_value() && !loaded.warning.isEmpty() && loaded.error.isEmpty()
                    && scheduler.preview().first().dueTick == 23,
                "invalid rhythm did not atomically fall back to built-in schedule");
    }
    const QString unreadablePath = directory.filePath("directory_instead_of_json");
    require(QDir().mkpath(unreadablePath), "could not create unreadable override case");
    loaded = loadAndReset(unreadablePath);
    require(loaded.plan.has_value() && !loaded.warning.isEmpty()
                && scheduler.preview().first().dueTick == 23,
            "unreadable external path did not fall back");

    loaded = loadAndReset(overridePath);
    require(loaded.plan.has_value(),
            "scheduler could not reset after invalid override");
    require(scheduler.spawnedCount() == 0 && scheduler.currentTick() == 0,
            "reset retained old dispatch state");

    AttackScheduler stopped;
    loaded = AttackPlanLoader::load(directory.filePath("missing_builtin.json"), overridePath);
    require(!loaded.plan.has_value() && !loaded.error.isEmpty()
                && loaded.warning.isEmpty() && !stopped.isReady(),
            "missing built-in roster provided a runnable plan");
    const QString malformedBuiltin = directory.filePath("malformed_builtin.json");
    writeText(malformedBuiltin, "{");
    loaded = AttackPlanLoader::load(malformedBuiltin, overridePath);
    require(!loaded.plan.has_value() && !loaded.error.isEmpty(),
            "malformed built-in roster provided a runnable plan");
    loaded = AttackPlanLoader::load(builtinPath, QString());
    require(loaded.plan.has_value() && loaded.warning.isEmpty() && loaded.error.isEmpty(),
            "an explicitly absent rhythm path did not keep defaults");

    std::cout << "PASS: product plan loader, isolated min-heap schedules, legacy trace and rhythm fallback\n";
    return 0;
}
