/*********************************************************************************************
 * Mozilla License
 * Just a meantime project to see the ability of qt, the framework that my OS might be based on
 * And for those linux users that believe in the power of notes
 *********************************************************************************************/

#include "updatebackend.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>
#include <QVersionNumber>

namespace {
constexpr auto SETTINGS_ORG = "Awesomeness";
constexpr auto SETTINGS_APP = "Settings";
constexpr auto UPDATES_URL = "https://raw.githubusercontent.com/nuttyartist/notes/master/UPDATES_FOSS.json";
constexpr auto DONT_SHOW_UPDATES_KEY = "dontShowUpdateWindow";
}  // namespace

UpdateBackend::UpdateBackend(QObject* parent)
    : QObject(parent),
      m_settings(QString::fromLatin1(SETTINGS_ORG), QString::fromLatin1(SETTINGS_APP)),
      m_titleText(tr("Checking for updates....")),
      m_changelogHtml(fallbackChangelogHtml()),
      m_downloadLabelText(tr("Downloading updates...")),
      m_timeRemainingText(QStringLiteral("%1: %2").arg(tr("Time remaining"), tr("unknown"))),
      m_dialogVisible(false),
      m_showProgressControls(false),
      m_progressIndeterminate(false),
      m_progressValue(0.0),
      m_dontNotifyAutomatically(m_settings.value(QString::fromLatin1(DONT_SHOW_UPDATES_KEY), false).toBool()),
      m_updateButtonEnabled(false),
      m_isChecking(false),
      m_isDownloading(false),
      m_hasUpdateAvailable(false),
      m_forceShowDialog(false) {}

bool UpdateBackend::enabled() const {
#if UPDATE_CHECKER
    return true;
#else
    return false;
#endif
}

bool UpdateBackend::dialogVisible() const {
    return m_dialogVisible;
}

QString UpdateBackend::titleText() const {
    return m_titleText;
}

QString UpdateBackend::installedVersion() const {
    return QCoreApplication::applicationVersion();
}

QString UpdateBackend::availableVersion() const {
    return m_availableVersion;
}

QString UpdateBackend::changelogHtml() const {
    return m_changelogHtml;
}

bool UpdateBackend::showProgressControls() const {
    return m_showProgressControls;
}

bool UpdateBackend::progressIndeterminate() const {
    return m_progressIndeterminate;
}

double UpdateBackend::progressValue() const {
    return m_progressValue;
}

QString UpdateBackend::downloadLabelText() const {
    return m_downloadLabelText;
}

QString UpdateBackend::timeRemainingText() const {
    return m_timeRemainingText;
}

bool UpdateBackend::dontNotifyAutomatically() const {
    return m_dontNotifyAutomatically;
}

void UpdateBackend::setDontNotifyAutomatically(bool value) {
    if (m_dontNotifyAutomatically == value) {
        return;
    }

    m_dontNotifyAutomatically = value;
    m_settings.setValue(QString::fromLatin1(DONT_SHOW_UPDATES_KEY), value);
    emit dontNotifyAutomaticallyChanged();
}

bool UpdateBackend::updateButtonEnabled() const {
    return m_updateButtonEnabled;
}

void UpdateBackend::checkForUpdates(bool force) {
    if (!enabled()) {
        if (force) {
            showFailureState(tr("Updates disabled"), tr("This build has the update checker disabled."), true);
        }
        return;
    }

    m_forceShowDialog = m_forceShowDialog || force;
    beginCheckingState();

    if (force) {
        setDialogVisible(true);
    }

    if (m_metadataReply != nullptr) {
        emit stateChanged();
        return;
    }

    QNetworkRequest request(QUrl(QString::fromLatin1(UPDATES_URL)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_metadataReply = m_networkManager.get(request);
    connect(m_metadataReply, &QNetworkReply::finished, this, &UpdateBackend::onMetadataFinished);
    emit stateChanged();
}

void UpdateBackend::requestUpdate() {
    if (!enabled() || !m_hasUpdateAvailable || m_isDownloading) {
        return;
    }

    if (m_downloadUrl.isEmpty()) {
        if (!m_openUrl.isEmpty()) {
            QDesktopServices::openUrl(QUrl(m_openUrl));
        }
        return;
    }

    startDownload(QUrl(m_downloadUrl));
}

void UpdateBackend::closeDialog() {
    setDialogVisible(false);
}

void UpdateBackend::onMetadataFinished() {
    const bool force = m_forceShowDialog;
    m_forceShowDialog = false;
    m_isChecking = false;

    QPointer<QNetworkReply> reply = m_metadataReply;
    m_metadataReply = nullptr;

    if (reply == nullptr) {
        emit stateChanged();
        return;
    }

    const QByteArray payload = reply->readAll();
    const auto networkError = reply->error();
    const QString errorString = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError) {
        showFailureState(tr("Unable to check for updates"),
                         tr("Could not retrieve update information: %1").arg(errorString), force);
        return;
    }

    QString parseError;
    const UpdateInfo updateInfo = parseUpdateInfo(payload, &parseError);
    if (!parseError.isEmpty()) {
        showFailureState(tr("Unable to check for updates"), parseError, force);
        return;
    }

    applyUpdateInfo(updateInfo, force);
}

void UpdateBackend::onDownloadProgress(qint64 received, qint64 total) {
    if (total > 0) {
        m_progressIndeterminate = false;
        m_progressValue = static_cast<double>(received * 100) / static_cast<double>(total);
        m_downloadLabelText =
            tr("Downloading updates") +
            QStringLiteral(" (%1 %2 %3)").arg(formatDataSize(received), tr("of"), formatDataSize(total));
        m_timeRemainingText = QStringLiteral("%1: %2").arg(tr("Time remaining"), formatTimeRemaining(received, total));
    } else {
        m_progressIndeterminate = true;
        m_progressValue = 0.0;
        m_downloadLabelText = tr("Downloading updates...");
        m_timeRemainingText = QStringLiteral("%1: %2").arg(tr("Time remaining"), tr("unknown"));
    }

    emit stateChanged();
}

void UpdateBackend::onDownloadFinished() {
    QPointer<QNetworkReply> reply = m_downloadReply;
    m_downloadReply = nullptr;
    m_isDownloading = false;

    if (reply == nullptr) {
        emit stateChanged();
        return;
    }

    const auto redirectTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    const auto networkError = reply->error();
    const QString errorString = reply->errorString();
    const QByteArray payload = reply->readAll();
    const QUrl replyUrl = reply->url();
    reply->deleteLater();

    if (!redirectTarget.isEmpty()) {
        const QUrl nextUrl = replyUrl.resolved(redirectTarget);
        startDownload(nextUrl);
        return;
    }

    if (networkError != QNetworkReply::NoError) {
        showFailureState(tr("Download failed"), tr("Could not download the update package: %1").arg(errorString), true);
        return;
    }

    const QString directoryPath = downloadDirectoryPath();
    QDir downloadDir(directoryPath);
    if (!downloadDir.exists() && !downloadDir.mkpath(QStringLiteral("."))) {
        showFailureState(tr("Download failed"), tr("Could not create the downloads directory."), true);
        return;
    }

    QString fileName = m_downloadFileName;
    if (fileName.isEmpty()) {
        fileName = QFileInfo(replyUrl.path()).fileName();
    }
    if (fileName.isEmpty()) {
        fileName = defaultDownloadFileName();
    }

    const QString filePath = downloadDir.filePath(fileName);
    QFile::remove(filePath);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        showFailureState(tr("Download failed"),
                         tr("Could not save the update package to %1.").arg(QDir::toNativeSeparators(filePath)), true);
        return;
    }

    file.write(payload);
    file.close();

    m_showProgressControls = true;
    m_progressIndeterminate = false;
    m_progressValue = 100.0;
    m_downloadLabelText = tr("Download finished!");
    m_timeRemainingText = tr("Opening downloaded file...");
    emit stateChanged();

    openDownloadedFile(filePath);
}

QString UpdateBackend::platformKey() const {
#if defined(Q_OS_WINDOWS)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("osx");
#else
    return QStringLiteral("linux");
#endif
}

UpdateBackend::UpdateInfo UpdateBackend::parseUpdateInfo(const QByteArray& payload, QString* errorMessage) const {
    UpdateInfo info;

    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (!document.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("The update metadata is not a valid JSON object.");
        }
        return info;
    }

    const QJsonObject rootObject = document.object();
    const QJsonObject updatesObject = rootObject.value(QStringLiteral("updates")).toObject();
    const QJsonObject platformObject = updatesObject.value(platformKey()).toObject();
    if (platformObject.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("No update metadata exists for this platform.");
        }
        return info;
    }

    info.latestVersion = platformObject.value(QStringLiteral("latest-version")).toString();
    info.changelog = platformObject.value(QStringLiteral("changelog")).toString();
    info.downloadUrl = platformObject.value(QStringLiteral("download-url")).toString();
    info.openUrl = platformObject.value(QStringLiteral("open-url")).toString();

    if (info.latestVersion.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = tr("The update metadata does not contain a latest version.");
        }
        return {};
    }

    return info;
}

bool UpdateBackend::isNewerVersionAvailable(const QString& latestVersion) const {
    qsizetype latestSuffixIndex = 0;
    qsizetype currentSuffixIndex = 0;
    const QVersionNumber latest = QVersionNumber::fromString(latestVersion, &latestSuffixIndex);
    const QVersionNumber current = QVersionNumber::fromString(installedVersion(), &currentSuffixIndex);

    if (!latest.isNull() && !current.isNull()) {
        return QVersionNumber::compare(latest, current) > 0;
    }

    return latestVersion.trimmed() != installedVersion().trimmed();
}

QString UpdateBackend::formatDataSize(qint64 bytes) const {
    if (bytes < 1024) {
        return tr("%1 bytes").arg(bytes);
    }

    if (bytes < (1024 * 1024)) {
        return tr("%1 KB").arg(QString::number(static_cast<double>(bytes) / 1024.0, 'f', 1));
    }

    return tr("%1 MB").arg(QString::number(static_cast<double>(bytes) / (1024.0 * 1024.0), 'f', 1));
}

QString UpdateBackend::formatTimeRemaining(qint64 received, qint64 total) const {
    if (received <= 0 || total <= 0 || !m_downloadTimer.isValid()) {
        return tr("unknown");
    }

    const qint64 elapsedMs = m_downloadTimer.elapsed();
    if (elapsedMs <= 0) {
        return tr("unknown");
    }

    const double bytesPerSecond = static_cast<double>(received) / (static_cast<double>(elapsedMs) / 1000.0);
    if (bytesPerSecond <= 0.0) {
        return tr("unknown");
    }

    const qint64 remainingBytes = qMax<qint64>(0, total - received);
    const qint64 secondsRemaining = qRound64(static_cast<double>(remainingBytes) / bytesPerSecond);

    if (secondsRemaining >= 7200) {
        return tr("about %1 hours").arg(qRound64(static_cast<double>(secondsRemaining) / 3600.0));
    }

    if (secondsRemaining >= 3600) {
        return tr("about one hour");
    }

    if (secondsRemaining >= 120) {
        return tr("%1 minutes").arg(qRound64(static_cast<double>(secondsRemaining) / 60.0));
    }

    if (secondsRemaining >= 60) {
        return tr("1 minute");
    }

    if (secondsRemaining > 1) {
        return tr("%1 seconds").arg(secondsRemaining);
    }

    return tr("1 second");
}

QString UpdateBackend::defaultDownloadFileName() const {
    return QStringLiteral("%1_Update_%2.bin").arg(QCoreApplication::applicationName(), m_availableVersion);
}

QString UpdateBackend::downloadDirectoryPath() const {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (!path.isEmpty()) {
        return path;
    }

    return QDir::homePath() + QStringLiteral("/Downloads");
}

QString UpdateBackend::fallbackChangelogHtml() const {
    return QStringLiteral("<p>No changelog found...</p>");
}

bool UpdateBackend::hasDownloadTarget() const {
    return !m_downloadUrl.isEmpty() || !m_openUrl.isEmpty();
}

void UpdateBackend::setDialogVisible(bool visible) {
    if (m_dialogVisible == visible) {
        return;
    }

    m_dialogVisible = visible;
    emit dialogVisibleChanged();
}

void UpdateBackend::resetProgressState() {
    m_showProgressControls = false;
    m_progressIndeterminate = false;
    m_progressValue = 0.0;
    m_downloadLabelText = tr("Downloading updates...");
    m_timeRemainingText = QStringLiteral("%1: %2").arg(tr("Time remaining"), tr("unknown"));
}

void UpdateBackend::beginCheckingState() {
    m_isChecking = true;
    m_titleText = tr("Checking for updates....");
    m_availableVersion.clear();
    m_downloadUrl.clear();
    m_openUrl.clear();
    m_downloadFileName.clear();
    m_changelogHtml = fallbackChangelogHtml();
    m_updateButtonEnabled = false;
    m_hasUpdateAvailable = false;
    resetProgressState();
}

void UpdateBackend::showFailureState(const QString& title, const QString& content, bool openDialog) {
    m_titleText = title;
    m_availableVersion.clear();
    m_changelogHtml = QStringLiteral("<p>%1</p>").arg(content.toHtmlEscaped());
    m_updateButtonEnabled = false;
    m_hasUpdateAvailable = false;
    resetProgressState();
    emit stateChanged();

    if (openDialog) {
        setDialogVisible(true);
    }
}

void UpdateBackend::applyUpdateInfo(const UpdateInfo& info, bool force) {
    m_availableVersion = info.latestVersion;
    m_downloadUrl = info.downloadUrl;
    m_openUrl = info.openUrl;
    m_changelogHtml = info.changelog.isEmpty() ? fallbackChangelogHtml() : info.changelog;
    m_hasUpdateAvailable = isNewerVersionAvailable(info.latestVersion);
    m_updateButtonEnabled = m_hasUpdateAvailable && hasDownloadTarget();
    resetProgressState();

    if (m_hasUpdateAvailable) {
        m_titleText = tr("A newer version is available!");
    } else {
        m_titleText = tr("You're up-to-date!");
    }

    emit stateChanged();

    if (m_hasUpdateAvailable) {
        if (force || !m_dontNotifyAutomatically) {
            setDialogVisible(true);
        }
        return;
    }

    if (force) {
        setDialogVisible(true);
    }
}

void UpdateBackend::startDownload(const QUrl& url) {
    if (!url.isValid()) {
        showFailureState(tr("Download failed"), tr("The update download URL is invalid."), true);
        return;
    }

    clearReply(m_downloadReply);

    m_isDownloading = true;
    m_showProgressControls = true;
    m_progressIndeterminate = true;
    m_progressValue = 0.0;
    m_downloadLabelText = tr("Downloading updates...");
    m_timeRemainingText = QStringLiteral("%1: %2").arg(tr("Time remaining"), tr("unknown"));
    m_updateButtonEnabled = false;
    m_downloadFileName = QFileInfo(url.path()).fileName();
    m_downloadTimer.restart();
    emit stateChanged();
    setDialogVisible(true);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    m_downloadReply = m_networkManager.get(request);
    connect(m_downloadReply, &QNetworkReply::downloadProgress, this, &UpdateBackend::onDownloadProgress);
    connect(m_downloadReply, &QNetworkReply::finished, this, &UpdateBackend::onDownloadFinished);
}

void UpdateBackend::openDownloadedFile(const QString& filePath) {
    if (QDesktopServices::openUrl(QUrl::fromLocalFile(filePath))) {
        QCoreApplication::quit();
        return;
    }

    openDownloadFolder(filePath);
    m_timeRemainingText = tr("Opened downloads folder instead.");
    emit stateChanged();
}

void UpdateBackend::openDownloadFolder(const QString& filePath) {
    const QFileInfo info(filePath);
    const QUrl folderUrl = QUrl::fromLocalFile(info.absolutePath());
    QDesktopServices::openUrl(folderUrl);
}

void UpdateBackend::clearReply(QPointer<QNetworkReply>& reply) {
    if (reply == nullptr) {
        return;
    }

    reply->abort();
    reply->deleteLater();
    reply = nullptr;
}
