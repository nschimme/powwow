#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H

#include <QString>
#include <QStringList>
#include <QSettings>

QString defaultPlatformEditor();
QString defaultPlatformPager();
QString profileDirectoryPath();
QString defaultPowFilePath();

struct SessionProfile {
    QString name;
    QString host = "mume.org";
    int port = 4242;
    QString powFilePath = defaultPowFilePath();
    QString editor = defaultPlatformEditor();
    QString pager = defaultPlatformPager();
    int scrollbackLines = 5000;
    bool useTls = true;
    bool useUtf8 = true;
};

class ProfileManager {
public:
    ProfileManager();

    QStringList sessionNames() const;
    SessionProfile loadSession(const QString &name) const;
    void saveSession(const SessionProfile &profile);
    void deleteSession(const QString &name);
};

#endif // PROFILEMANAGER_H
