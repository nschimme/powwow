#include "profilemanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

QString defaultPlatformEditor()
{
#if defined(Q_OS_WIN)
    return "notepad";
#elif defined(Q_OS_MAC)
    return "open -W -n -t";
#else
    return "xdg-open";
#endif
}

QString defaultPlatformPager()
{
#if defined(Q_OS_WIN)
    return "notepad";
#elif defined(Q_OS_MAC)
    return "open -W -n -t";
#else
    return "xdg-open";
#endif
}

QString profileDirectoryPath()
{
    QString docsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docsPath.isEmpty()) {
        docsPath = QDir::homePath();
    }
    QString profileDir = docsPath + "/mume";
    QDir().mkpath(profileDir);
    return profileDir;
}

QString defaultPowFilePath()
{
    QString dir = profileDirectoryPath();
    QString filePath = dir + "/default.pow";

    if (!QFile::exists(filePath)) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "# Powwow definition file for MUME\n";
            out << "#host mume.org 4242\n";
            file.close();
        }
    }
    return filePath;
}

ProfileManager::ProfileManager()
{
}

QStringList ProfileManager::sessionNames() const
{
    QSettings settings("mume", "powtty");
    QStringList names;
    const auto childGroups = settings.childGroups();
    for (const QString &group : childGroups) {
        if (group.startsWith("session_")) {
            names.append(group.mid(8));
        }
    }
    return names;
}

SessionProfile ProfileManager::loadSession(const QString &name) const
{
    QSettings settings("mume", "powtty");
    SessionProfile profile;
    profile.name = name;
    QString groupKey = QString("session_%1").arg(name);

    if (settings.childGroups().contains(groupKey)) {
        settings.beginGroup(groupKey);
        profile.host = settings.value("host", "mume.org").toString();
        profile.port = settings.value("port", 4242).toInt();
        profile.powFilePath = settings.value("powFilePath", defaultPowFilePath()).toString();
        profile.editor = settings.value("editor", defaultPlatformEditor()).toString();
        profile.pager = settings.value("pager", defaultPlatformPager()).toString();
        profile.scrollbackLines = settings.value("scrollbackLines", 5000).toInt();
        profile.useTls = settings.value("useTls", true).toBool();
        profile.useUtf8 = settings.value("useUtf8", true).toBool();
        profile.fontFamily = settings.value("fontFamily", "PowTTY Mono").toString();
        profile.fontSize = settings.value("fontSize", 10).toInt();
        settings.endGroup();
    }
    return profile;
}

void ProfileManager::saveSession(const SessionProfile &profile)
{
    if (profile.name.trimmed().isEmpty()) {
        return;
    }
    QSettings settings("mume", "powtty");
    QString groupKey = QString("session_%1").arg(profile.name.trimmed());
    settings.beginGroup(groupKey);
    settings.setValue("host", profile.host);
    settings.setValue("port", profile.port);
    settings.setValue("powFilePath", profile.powFilePath);
    settings.setValue("editor", profile.editor);
    settings.setValue("pager", profile.pager);
    settings.setValue("scrollbackLines", profile.scrollbackLines);
    settings.setValue("useTls", profile.useTls);
    settings.setValue("useUtf8", profile.useUtf8);
    settings.setValue("fontFamily", profile.fontFamily);
    settings.setValue("fontSize", profile.fontSize);
    settings.endGroup();
    settings.sync();
}

void ProfileManager::deleteSession(const QString &name)
{
    QSettings settings("mume", "powtty");
    QString groupKey = QString("session_%1").arg(name);
    settings.remove(groupKey);
    settings.sync();
}
