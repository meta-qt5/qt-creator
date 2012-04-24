/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBIANMANAGER_H
#define DEBIANMANAGER_H

#include <coreplugin/id.h>
#include <utils/fileutils.h>

#include <QObject>
#include <QHash>

namespace Utils { class FileSystemWatcher; }
namespace ProjectExplorer { class Target; }
namespace Qt4ProjectManager { class Qt4BuildConfiguration; }

namespace Madde {
namespace Internal {
class MaddePlugin;

class DebianManager : public QObject
{
    Q_OBJECT
public:
    ~DebianManager();

    static DebianManager *instance();

    // ref counted:
    void monitor(const Utils::FileName &debianDir);
    bool isMonitoring(const Utils::FileName &debianDir);
    void ignore(const Utils::FileName &debianDir);

    static QString projectVersion(const Utils::FileName &debianDir, QString *error = 0);
    static bool setProjectVersion(const Utils::FileName &debianDir, const QString &version, QString *error = 0);
    static QString packageName(const Utils::FileName &debianDir);
    static bool setPackageName(const Utils::FileName &debianDir, const QString &packageName);
    static QString shortDescription(const Utils::FileName &debianDir);
    static bool setShortDescription(const Utils::FileName &debianDir, const QString &description);
    static QString packageManagerName(const Utils::FileName &debianDir, Core::Id deviceType);
    static bool setPackageManagerName(const Utils::FileName &debianDir, Core::Id deviceType, const QString &name);
    static QIcon packageManagerIcon(const Utils::FileName &debianDir, QString *error = 0);
    static bool setPackageManagerIcon(const Utils::FileName &debianDir, Core::Id deviceType,
                                      const Utils::FileName &iconPath, QString *error = 0);
    static bool hasPackageManagerIcon(const Utils::FileName &debianDir);

    static Utils::FileName packageFileName(const Utils::FileName &debianDir);

    enum ActionStatus { NoActionRequired, ActionSuccessful, ActionFailed };
    // will not start to monitor this new dir!
    static ActionStatus createTemplate(Qt4ProjectManager::Qt4BuildConfiguration *bc,
                                       const Utils::FileName &debianDir);

    static QStringList debianFiles(const Utils::FileName &debianDir);

    static Utils::FileName debianDirectory(ProjectExplorer::Target *target);

signals:
    void debianDirectoryChanged(const Utils::FileName &dir);
    void changelogChanged(const Utils::FileName &dir);
    void controlChanged(const Utils::FileName &dir);

private slots:
    void directoryWasChanged(const QString &path);
    void controlWasChanged();
    void changelogWasChanged();

private:
    explicit DebianManager(QObject *parent = 0);

    static Utils::FileName changelogFilePath(const Utils::FileName &debianDir);
    static Utils::FileName controlFilePath(const Utils::FileName &debianDir);
    static Utils::FileName rulesFilePath(const Utils::FileName &debianDir);

    Utils::FileSystemWatcher *m_watcher;
    QHash<Utils::FileName, int> m_watches;
    static DebianManager *m_instance;

    friend class MaddePlugin;
};

} // namespace Internal
} // namespace Madde

#endif // DEBIANMANAGER_H
