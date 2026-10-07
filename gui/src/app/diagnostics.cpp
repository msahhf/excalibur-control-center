#include "diagnostics.h"

#include "app/identity.h"
#include "app/usersettings.h"
#include "app/version.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>
#include <QSysInfo>

namespace {

// Process-wide uptime clock (started on first use, i.e. effectively app start).
qint64 uptimeSeconds()
{
    static QElapsedTimer timer = []() {
        QElapsedTimer t;
        t.start();
        return t;
    }();
    return timer.elapsed() / 1000;
}

QString buildType()
{
#ifdef EXCALIBUR_BUILD_TYPE
    const QString t = QString::fromLatin1(EXCALIBUR_BUILD_TYPE);
    if (!t.isEmpty())
        return t;
#endif
#ifdef QT_NO_DEBUG
    return QStringLiteral("Release");
#else
    return QStringLiteral("Debug");
#endif
}

QString stateString(AppState::Status status)
{
    switch (status) {
    case AppState::Status::Connected: return QStringLiteral("Connected");
    case AppState::Status::Degraded: return QStringLiteral("Degraded");
    case AppState::Status::Disconnected: break;
    }
    return QStringLiteral("Unavailable");
}

QString themeString(UserSettings::AppTheme theme)
{
    return UserSettings::themeToString(theme);
}

// Rounds to 1 decimal so the JSON stays human-readable without losing the value.
QJsonValue tempValue(bool valid, double celsius)
{
    if (!valid)
        return QJsonValue(QJsonValue::Null);
    return QJsonValue(qRound(celsius * 10.0) / 10.0);
}

QJsonValue intValue(bool valid, int v)
{
    return valid ? QJsonValue(v) : QJsonValue(QJsonValue::Null);
}

QJsonValue strValue(const QString &s)
{
    return s.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(s);
}

} // namespace

DiagnosticsSnapshot buildDiagnostics(const TelemetrySnapshot &t,
                                     const QVector<HistorySample> &history,
                                     const SystemInfo &sys,
                                     const DiagnosticsContext &context)
{
    DiagnosticsSnapshot d;

    d.productName = AppIdentity::displayName();
    d.version = QString::fromLatin1(AppVersion::string());
    d.buildType = buildType();
    d.qtVersion = QString::fromLatin1(qVersion());
    d.uptimeSeconds = uptimeSeconds();

    d.os = QSysInfo::prettyProductName();
    d.kernel = QStringLiteral("%1 %2").arg(sys.kernelType, sys.kernelVersion).trimmed();
    d.architecture = sys.architecture.isEmpty() ? QSysInfo::currentCpuArchitecture() : sys.architecture;
    // Only the desktop/session identifiers — never the whole environment.
    d.desktopSession = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    d.sessionType = qEnvironmentVariable("XDG_SESSION_TYPE");

    d.state = stateString(t.status);
    d.cpuTempValid = t.cpuTempValid;
    d.cpuTempC = t.cpuTempC;
    d.gpuTempValid = t.gpuTempValid;
    d.gpuTempC = t.gpuTempC;
    d.cpuFanValid = t.cpuFanValid;
    d.cpuFanRpm = t.cpuFanRpm;
    d.gpuFanValid = t.gpuFanValid;
    d.gpuFanRpm = t.gpuFanRpm;
    d.refreshMs = UserSettings::instance().refreshMs();
    d.snapshotTimestamp = QDateTime::currentDateTime().toString(Qt::ISODate);

    d.currentPage = context.currentPage;
    d.trayAvailable = context.trayAvailable;
    d.trayVisible = context.trayVisible;
    d.singleInstance = context.singleInstance;

    d.theme = themeString(UserSettings::instance().theme());
    d.temperatureUnit = UserSettings::tempUnitToString(UserSettings::instance().tempUnit());

    d.sensorsAvailable = t.deviceFound && !t.hwmonPath.isEmpty();
    d.hwmonPath = t.hwmonPath;
    d.deviceName = t.deviceFound ? QStringLiteral("excalibur_g870") : QString();
    d.cpuLabel = t.cpuLabel;
    d.gpuLabel = t.gpuLabel;
    d.cpuFanLabel = t.cpuFanLabel;
    d.gpuFanLabel = t.gpuFanLabel;

    // History: the model stores monotonic timestamps; anchor the newest sample to
    // the wall clock so exported timestamps are real ISO times with correct order.
    qint64 newest = 0;
    for (const HistorySample &s : history) {
        if (s.timestampMs > newest)
            newest = s.timestampMs;
    }
    const qint64 anchorWallMs = QDateTime::currentMSecsSinceEpoch();
    d.history.reserve(history.size());
    for (const HistorySample &s : history) {
        DiagnosticsHistorySample h;
        const qint64 wall = anchorWallMs - (newest - s.timestampMs);
        h.timestamp = QDateTime::fromMSecsSinceEpoch(wall).toString(Qt::ISODate);
        h.cpuTempValid = s.cpuTempValid;
        h.cpuTempC = s.cpuTempC;
        h.gpuTempValid = s.gpuTempValid;
        h.gpuTempC = s.gpuTempC;
        h.cpuFanValid = s.cpuFanValid;
        h.cpuFanRpm = s.cpuFanRpm;
        h.gpuFanValid = s.gpuFanValid;
        h.gpuFanRpm = s.gpuFanRpm;
        d.history.append(h);
    }

    return d;
}

QJsonObject DiagnosticsSnapshot::toJson() const
{
    QJsonObject root;

    QJsonObject app;
    app.insert(QStringLiteral("product_name"), productName);
    app.insert(QStringLiteral("version"), version);
    app.insert(QStringLiteral("build_type"), buildType);
    app.insert(QStringLiteral("qt_version"), qtVersion);
    app.insert(QStringLiteral("uptime_seconds"), double(uptimeSeconds));
    root.insert(QStringLiteral("application"), app);

    QJsonObject system;
    system.insert(QStringLiteral("os"), strValue(os));
    system.insert(QStringLiteral("kernel"), strValue(kernel));
    system.insert(QStringLiteral("architecture"), strValue(architecture));
    system.insert(QStringLiteral("desktop_session"), strValue(desktopSession));
    system.insert(QStringLiteral("session_type"), strValue(sessionType));
    root.insert(QStringLiteral("system"), system);

    QJsonObject telemetry;
    telemetry.insert(QStringLiteral("state"), state);
    telemetry.insert(QStringLiteral("cpu_temperature_c"), tempValue(cpuTempValid, cpuTempC));
    telemetry.insert(QStringLiteral("gpu_temperature_c"), tempValue(gpuTempValid, gpuTempC));
    telemetry.insert(QStringLiteral("cpu_fan_rpm"), intValue(cpuFanValid, cpuFanRpm));
    telemetry.insert(QStringLiteral("gpu_fan_rpm"), intValue(gpuFanValid, gpuFanRpm));
    telemetry.insert(QStringLiteral("refresh_interval_ms"), refreshMs);
    telemetry.insert(QStringLiteral("snapshot_timestamp"), snapshotTimestamp);
    telemetry.insert(QStringLiteral("history_samples"), history.size());
    telemetry.insert(QStringLiteral("history_window_seconds"), historyWindowSeconds);
    root.insert(QStringLiteral("telemetry"), telemetry);

    QJsonObject runtime;
    runtime.insert(QStringLiteral("current_page"), strValue(currentPage));
    runtime.insert(QStringLiteral("tray_available"), trayAvailable);
    runtime.insert(QStringLiteral("tray_visible"), trayVisible);
    runtime.insert(QStringLiteral("single_instance"), singleInstance);
    runtime.insert(QStringLiteral("theme"), theme);
    runtime.insert(QStringLiteral("temperature_unit"), temperatureUnit);
    root.insert(QStringLiteral("runtime"), runtime);

    QJsonObject settings;
    settings.insert(QStringLiteral("theme"), theme);
    settings.insert(QStringLiteral("temperature_unit"), temperatureUnit);
    settings.insert(QStringLiteral("refresh_interval_ms"), refreshMs);
    root.insert(QStringLiteral("settings"), settings);

    QJsonObject sensors;
    sensors.insert(QStringLiteral("available"), sensorsAvailable);
    sensors.insert(QStringLiteral("hwmon_device"), strValue(hwmonPath));
    sensors.insert(QStringLiteral("device_name"), strValue(deviceName));
    const auto channel = [](const QString &label, const QString &source) {
        QJsonObject o;
        o.insert(QStringLiteral("label"), strValue(label));
        o.insert(QStringLiteral("source"), source);
        return o;
    };
    // Source file names match exactly what HwmonClient reads (temp1/2_input,
    // fan1/2_input); they are not guessed.
    sensors.insert(QStringLiteral("cpu"), channel(cpuLabel, QStringLiteral("temp1_input")));
    sensors.insert(QStringLiteral("gpu"), channel(gpuLabel, QStringLiteral("temp2_input")));
    sensors.insert(QStringLiteral("cpu_fan"), channel(cpuFanLabel, QStringLiteral("fan1_input")));
    sensors.insert(QStringLiteral("gpu_fan"), channel(gpuFanLabel, QStringLiteral("fan2_input")));
    root.insert(QStringLiteral("sensors"), sensors);

    QJsonObject historyObj;
    historyObj.insert(QStringLiteral("window_seconds"), historyWindowSeconds);
    QJsonArray samples;
    for (const DiagnosticsHistorySample &h : history) {
        QJsonObject o;
        o.insert(QStringLiteral("timestamp"), strValue(h.timestamp));
        o.insert(QStringLiteral("cpu_temperature_c"), tempValue(h.cpuTempValid, h.cpuTempC));
        o.insert(QStringLiteral("gpu_temperature_c"), tempValue(h.gpuTempValid, h.gpuTempC));
        o.insert(QStringLiteral("cpu_fan_rpm"), intValue(h.cpuFanValid, h.cpuFanRpm));
        o.insert(QStringLiteral("gpu_fan_rpm"), intValue(h.gpuFanValid, h.gpuFanRpm));
        samples.append(o);
    }
    historyObj.insert(QStringLiteral("samples"), samples);
    root.insert(QStringLiteral("history"), historyObj);

    return root;
}

QString diagnosticsJson(const DiagnosticsSnapshot &snapshot)
{
    return QString::fromUtf8(QJsonDocument(snapshot.toJson()).toJson(QJsonDocument::Indented));
}

bool saveDiagnostics(const DiagnosticsSnapshot &snapshot, const QString &path, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    const QByteArray bytes = QJsonDocument(snapshot.toJson()).toJson(QJsonDocument::Indented);
    if (f.write(bytes) != bytes.size()) {
        if (error)
            *error = f.errorString();
        return false;
    }
    f.close();
    return true;
}

QString diagnosticsFileName()
{
    return QStringLiteral("excalibur-control-center-diagnostics-%1.json")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
}
