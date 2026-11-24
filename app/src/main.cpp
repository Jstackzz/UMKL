// MK Arcade Online: the arcade Mortal Kombat games (MK1, MK2, MK3, UMK3) from the player's own ROM sets, in one
// controller-driven app: the menus, the games and rollback netplay in the same window.
//
//   MKArcade.exe                       the app
//   MKArcade.exe --net-test set=umk3 side=1 port=7101 peer=127.0.0.1:7102 delay=2 frames=3000 log=a.txt
//                                      two of these (sides 1 and 2) play scripted inputs against each other and
//                                      write their states' hashes every 300 frames: the two logs must match
//   MKArcade.exe --online-test name=a pass=secret game=umk3 ranked=0 matches=1 autoplay=attack
//                [server=127.0.0.1:7777 fp=<sha256 hex>] [port=7201] [roms=<folder>] [timeout=300]
//                [lobby=host|join [mode=kombat|sparring] [codefile=<file>]] [leave_after=<seconds>]
//                                      without a window: signs in (creating the account), searches, accepts and
//                                      plays the match with a script ("attack" walks in and punches, "idle" stands);
//                                      two of these, each with its own MKA_DATA_DIR, play a whole online match (or a
//                                      lobby's: one hosts, the other joins). tools/py/local_server.py runs a server
#include "GameSession.h"
#include "GameView.h"
#include "InputHub.h"
#include "Library.h"
#include "Settings.h"
#include "online/AppPaths.h"
#include "online/GameProcess.h"
#include "online/OnlineHub.h"

#include <QFont>
#include <QGuiApplication>
#include <QMap>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

#include <cstdio>

static int NetTest(QGuiApplication& app, const QStringList& args)
{
    QMap<QString, QString> kv;
    for (const QString& a : args)
        if (a.contains('='))
            kv[a.section('=', 0, 0)] = a.section('=', 1);
    Settings settings;
    if (kv.contains("roms"))
        settings.overrideRomDir(kv.value("roms"));
    GameSession session(nullptr);
    QObject::connect(&session, &GameSession::ended, &app, [&](const QString& why) {
        fprintf(stdout, "%s\n", qPrintable(why));
        fflush(stdout);
        app.exit(why == QLatin1String("test finished") ? 0 : 1);
    });
    session.playNetTest(kv.value("set", "umk3"), kv.value("side", "1").toInt(), kv.value("port", "7101").toInt(),
                        kv.value("peer", "127.0.0.1:7102"), kv.value("delay", "2").toInt(),
                        kv.value("frames", "3000").toInt(), kv.value("log"));
    const int rc = app.exec();
    session.stop();
    return rc;
}

static int OnlineTest(QGuiApplication& app, const QStringList& args)
{
    QMap<QString, QString> kv;
    for (const QString& a : args)
        if (a.contains('='))
            kv[a.section('=', 0, 0)] = a.section('=', 1);
    Settings settings;
    if (kv.contains("roms"))
        settings.overrideRomDir(kv.value("roms"));
    if (kv.contains("server"))
        settings.overrideServer(kv.value("server"), kv.value("fp"));
    if (kv.contains("port"))
        settings.overrideNetPort(kv.value("port").toInt());
    settings.setVolume(0);
    auto out = [](const QString& line) {
        fprintf(stdout, "%s\n", qPrintable(line));
        fflush(stdout);
    };
    GameSession session(nullptr);
    session.setAutoplay(kv.value("autoplay", "idle"));
    GameProcess::setSession(&session);
    int rc = 2;
    {
        OnlineHub hub;
        // leave_after=N: N seconds into a match its player leaves it (the pause menu's LEAVE MATCH), as Online
        // Practice ends
        const int leaveAfter = kv.value("leave_after", "0").toInt();
        QObject::connect(&session, &GameSession::linkStatus, &hub, [&](const QString& line, quint64) {
            out(QStringLiteral("game: ") + line);
            if (leaveAfter > 0 && line.startsWith(QLatin1String("in_match")))
                QTimer::singleShot(leaveAfter * 1000, &hub, [&] {
                    out(QStringLiteral("test: leaving the match"));
                    hub.standDown();
                    session.stop();
                });
        });
        QObject::connect(&hub, &OnlineHub::notice, &app, [&](const QString& t) { out(QStringLiteral("notice: ") + t); });
        QObject::connect(&hub, &OnlineHub::matchChanged, &app, [&] {
            static QString last;
            const QString now = hub.phase() + QStringLiteral(": ") + hub.phaseText();
            if (now != last)
                out(QStringLiteral("phase ") + now);
            last = now;
        });
        QObject::connect(&hub, &OnlineHub::testFinished, &app, [&](bool ok, const QString& why) {
            out((ok ? QStringLiteral("TEST OK: ") : QStringLiteral("TEST FAILED: ")) + why);
            // the game parks and the server hears the series is left before the app goes
            QTimer::singleShot(3000, &app, [&app, ok] { app.exit(ok ? 0 : 1); });
        });
        QTimer::singleShot(kv.value("timeout", "300").toInt() * 1000, &app, [&] {
            out(QStringLiteral("TEST FAILED: timed out in phase ") + hub.phase());
            app.exit(2);
        });
        hub.runTest(kv.value("name"), kv.value("pass"), kv.value("game", "umk3"), kv.value("ranked") == "1",
                    kv.value("matches", "1").toInt(), kv.value("lobby"), kv.value("mode", "kombat"),
                    kv.value("codefile"));
        rc = app.exec();
    }
    session.stop();
    return rc;
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("MK Arcade Online"));
    app.setOrganizationName(QString());

    const QStringList args = app.arguments();
    if (args.contains(QStringLiteral("--net-test")))
        return NetTest(app, args);
    if (args.contains(QStringLiteral("--online-test")))
        return OnlineTest(app, args);

    QFont font(QStringLiteral("Bahnschrift"));
    font.setPixelSize(18);
    app.setFont(font);

    Settings settings;
    InputHub input;
    GameSession session(&input);
    Library library(&settings);
    GameProcess::setSession(&session);
    OnlineHub online;  // after the session: its matches' games close before it does

    qmlRegisterType<GameView>("MKArcade.Native", 1, 0, "GameView");
    qmlRegisterUncreatableType<GameSession>("MKArcade.Native", 1, 0, "GameSession", "use the session object");

    // development: --shot out.png [--nav accept,right,down] [--wait 4000] [--size 1920x1080]: start windowed, press
    // those buttons one by one, save the window as a picture and quit (looking at a screen without a person)
    const int shotAt = args.indexOf(QStringLiteral("--shot"));
    auto arg = [&](const char* name, const QString& fallback) {
        const int i = args.indexOf(QLatin1String(name));
        return i >= 0 && i + 1 < args.size() ? args.at(i + 1) : fallback;
    };
    if (shotAt >= 0)
        settings.overrideFullscreen(false);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("appSettings", &settings);
    engine.rootContext()->setContextProperty("input", &input);
    engine.rootContext()->setContextProperty("emu", &session);
    engine.rootContext()->setContextProperty("library", &library);
    engine.rootContext()->setContextProperty("online", &online);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("MKArcade", "Main");

    if (shotAt >= 0 && !engine.rootObjects().isEmpty()) {
        auto* win = qobject_cast<QQuickWindow*>(engine.rootObjects().first());
        const QStringList size = arg("--size", "1600x900").split('x');
        if (win && size.size() == 2)
            win->resize(size[0].toInt(), size[1].toInt());
        const QString out = arg("--shot", "shot.png");
        // steps: menu buttons (accept, down...), "wait:<ms>", and keys a player presses in a game ("key:enter",
        // "key:u": pressed for a tenth of a second, as the keyboard would)
        const QStringList steps = arg("--nav", "").split(',', Qt::SkipEmptyParts);
        const int wait = arg("--wait", "4000").toInt();
        static const QMap<QString, int> keys = {{"enter", Qt::Key_Return}, {"u", Qt::Key_U}, {"j", Qt::Key_J},
                                                {"i", Qt::Key_I}, {"left", Qt::Key_Left}, {"right", Qt::Key_Right},
                                                {"up", Qt::Key_Up}, {"down", Qt::Key_Down}};
        int t = wait;
        for (const QString& s : steps) {
            if (s.startsWith(QLatin1String("wait:"))) {
                t += s.mid(5).toInt();
                continue;
            }
            if (s.startsWith(QLatin1String("key:"))) {
                const int key = keys.value(s.mid(4), 0);
                QTimer::singleShot(t, &input, [&input, key] { input.keyEvent(key, true, false); });
                QTimer::singleShot(t + 100, &input, [&input, key] { input.keyEvent(key, false, false); });
            } else {
                QTimer::singleShot(t, &input, [&input, s] { emit input.nav(s); });
            }
            t += 700;
        }
        QTimer::singleShot(t + wait, &app, [win, out, &app, &session] {
            win->grabWindow().save(out);
            QImage frame;
            quint64 serial = 0;
            if (session.frames()->take(frame, serial))
                frame.save(out + ".frame.png");
            app.quit();
        });
    }
    const int rc = app.exec();
    session.stop();
    return rc;
}
