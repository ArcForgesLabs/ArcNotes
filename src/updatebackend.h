/*********************************************************************************************
 * Mozilla License
 * Just a meantime project to see the ability of qt, the framework that my OS might be based on
 * And for those linux users that believe in the power of notes
 *********************************************************************************************/

#ifndef UPDATEBACKEND_H
#define UPDATEBACKEND_H

#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QSettings>
#include <QtQml/qqmlregistration.h>

class QNetworkReply;
class QUrl;

class UpdateBackend : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool enabled READ enabled CONSTANT)
    Q_PROPERTY(bool dialogVisible READ dialogVisible NOTIFY dialogVisibleChanged)
    Q_PROPERTY(QString titleText READ titleText NOTIFY stateChanged)
    Q_PROPERTY(QString installedVersion READ installedVersion CONSTANT)
    Q_PROPERTY(QString availableVersion READ availableVersion NOTIFY stateChanged)
    Q_PROPERTY(QString changelogHtml READ changelogHtml NOTIFY stateChanged)
    Q_PROPERTY(bool showProgressControls READ showProgressControls NOTIFY stateChanged)
    Q_PROPERTY(bool progressIndeterminate READ progressIndeterminate NOTIFY stateChanged)
    Q_PROPERTY(double progressValue READ progressValue NOTIFY stateChanged)
    Q_PROPERTY(QString downloadLabelText READ downloadLabelText NOTIFY stateChanged)
    Q_PROPERTY(QString timeRemainingText READ timeRemainingText NOTIFY stateChanged)
    Q_PROPERTY(bool dontNotifyAutomatically READ dontNotifyAutomatically WRITE setDontNotifyAutomatically NOTIFY dontNotifyAutomaticallyChanged)
    Q_PROPERTY(bool updateButtonEnabled READ updateButtonEnabled NOTIFY stateChanged)

public:
    explicit UpdateBackend(QObject* parent = nullptr);

    [[nodiscard]] bool enabled() const;
    [[nodiscard]] bool dialogVisible() const;
    [[nodiscard]] QString titleText() const;
    [[nodiscard]] QString installedVersion() const;
    [[nodiscard]] QString availableVersion() const;
    [[nodiscard]] QString changelogHtml() const;
    [[nodiscard]] bool showProgressControls() const;
    [[nodiscard]] bool progressIndeterminate() const;
    [[nodiscard]] double progressValue() const;
    [[nodiscard]] QString downloadLabelText() const;
    [[nodiscard]] QString timeRemainingText() const;
    [[nodiscard]] bool dontNotifyAutomatically() const;
    void setDontNotifyAutomatically(bool value);
    [[nodiscard]] bool updateButtonEnabled() const;

    Q_INVOKABLE void checkForUpdates(bool force);
    Q_INVOKABLE void requestUpdate();
    Q_INVOKABLE void closeDialog();

signals:
    void stateChanged();
    void dialogVisibleChanged();
    void dontNotifyAutomaticallyChanged();

private slots:
    void onMetadataFinished();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadFinished();

private:
    struct UpdateInfo {
        QString latestVersion;
        QString changelog;
        QString downloadUrl;
        QString openUrl;
    };

    [[nodiscard]] QString platformKey() const;
    [[nodiscard]] UpdateInfo parseUpdateInfo(const QByteArray& payload, QString* errorMessage) const;
    [[nodiscard]] bool isNewerVersionAvailable(const QString& latestVersion) const;
    [[nodiscard]] QString formatDataSize(qint64 bytes) const;
    [[nodiscard]] QString formatTimeRemaining(qint64 received, qint64 total) const;
    [[nodiscard]] QString defaultDownloadFileName() const;
    [[nodiscard]] QString downloadDirectoryPath() const;
    [[nodiscard]] QString fallbackChangelogHtml() const;
    [[nodiscard]] bool hasDownloadTarget() const;
    void setDialogVisible(bool visible);
    void resetProgressState();
    void beginCheckingState();
    void showFailureState(const QString& title, const QString& content, bool openDialog);
    void applyUpdateInfo(const UpdateInfo& info, bool force);
    void startDownload(const QUrl& url);
    void openDownloadedFile(const QString& filePath);
    void openDownloadFolder(const QString& filePath);
    void clearReply(QPointer<QNetworkReply>& reply);

    QSettings m_settings;
    QNetworkAccessManager m_networkManager;
    QPointer<QNetworkReply> m_metadataReply;
    QPointer<QNetworkReply> m_downloadReply;
    QElapsedTimer m_downloadTimer;
    QString m_titleText;
    QString m_availableVersion;
    QString m_changelogHtml;
    QString m_downloadLabelText;
    QString m_timeRemainingText;
    QString m_downloadUrl;
    QString m_openUrl;
    QString m_downloadFileName;
    bool m_dialogVisible;
    bool m_showProgressControls;
    bool m_progressIndeterminate;
    double m_progressValue;
    bool m_dontNotifyAutomatically;
    bool m_updateButtonEnabled;
    bool m_isChecking;
    bool m_isDownloading;
    bool m_hasUpdateAvailable;
    bool m_forceShowDialog;
};

#endif  // UPDATEBACKEND_H
