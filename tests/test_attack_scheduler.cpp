#include "../26C-/attackscheduler.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>

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
    QCoreApplication app(argc, argv);
    AttackScheduler scheduler;
    QString warning;
    QString error;
    QTemporaryDir directory;
    require(directory.isValid(), "temporary directory failed");
    const QString overridePath = directory.filePath("attack_rhythm.json");
    require(scheduler.reset(overridePath, &warning, &error), "default schedule did not load");
    require(warning.isEmpty() && error.isEmpty(), "missing override was not silent");
    require(scheduler.totalEnemies() == 90, "default roster is not 90 enemies");

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

    require(scheduler.reset(overridePath), "reset after full schedule failed");
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
    require(scheduler.reset(overridePath, &warning, &error) && warning.isEmpty(),
            "valid rhythm override was rejected");
    require(scheduler.preview().first().dueTick == 5,
            "valid rhythm override did not change first spawn");

    writeText(overridePath,
              R"({"version":1,"waves":[{"openingDelayFrames":5},{"burstGapFrames":1.5},{},{},{},{}]})");
    require(scheduler.reset(overridePath, &warning, &error) && !warning.isEmpty(),
            "fractional rhythm override was not rejected");
    require(scheduler.preview().first().dueTick == 23,
            "invalid override partly changed the built-in schedule");

    writeText(overridePath,
              R"({"version":1,"waves":[{"groups":[]},{},{},{},{},{}]})");
    require(scheduler.reset(overridePath, &warning, &error) && !warning.isEmpty(),
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
        require(scheduler.reset(overridePath, &warning, &error)
                    && !warning.isEmpty()
                    && scheduler.preview().first().dueTick == 23,
                "invalid rhythm did not atomically fall back to built-in schedule");
    }
    const QString unreadablePath = directory.filePath("directory_instead_of_json");
    require(QDir().mkpath(unreadablePath), "could not create unreadable override case");
    require(scheduler.reset(unreadablePath, &warning, &error)
                && !warning.isEmpty() && scheduler.preview().first().dueTick == 23,
            "unreadable external path did not fall back");

    require(scheduler.reset(overridePath, &warning, &error),
            "scheduler could not reset after invalid override");
    require(scheduler.spawnedCount() == 0 && scheduler.currentTick() == 0,
            "reset retained old dispatch state");

    require(!scheduler.reset(overridePath, &warning, &error,
                             directory.filePath("missing_builtin.json"))
                && !error.isEmpty() && !scheduler.isReady(),
            "missing built-in roster did not stop the scheduler");

    std::cout << "PASS: product min-heap schedule, legacy trace, wave gate and rhythm fallback\n";
    return 0;
}
