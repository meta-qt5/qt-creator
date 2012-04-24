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
#ifndef REMOTELINUXRUNCONFIGURATIONFACTORY_H
#define REMOTELINUXRUNCONFIGURATIONFACTORY_H

#include <projectexplorer/runconfiguration.h>
#include <qt4projectmanager/qmakerunconfigurationfactory.h>

namespace ProjectExplorer { class Node; }

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxRunConfigurationFactory : public Qt4ProjectManager::QmakeRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit RemoteLinuxRunConfigurationFactory(QObject *parent = 0);
    ~RemoteLinuxRunConfigurationFactory();

    QString displayNameForId(const Core::Id id) const;
    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const;

    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id);

    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent,
        const QVariantMap &map);

    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent,
        ProjectExplorer::RunConfiguration *source);

    bool canHandle(ProjectExplorer::Target *t) const;
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Target *t,
                                                                        ProjectExplorer::Node *n);
};

} // namespace Internal
} // namespace RemoteLinux

#endif // REMOTELINUXRUNCONFIGURATIONFACTORY_H
