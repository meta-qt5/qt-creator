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

#include "targetsetuppage.h"
#include "importwidget.h"

#include "ui_targetsetuppage.h"
#include "buildconfigurationinfo.h"
#include "qt4buildconfiguration.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qmakeprofileinformation.h"

#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/qtprofileinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionfactory.h>
#include <utils/qtcprocess.h>

#include <QLabel>
#include <QMessageBox>

using namespace Qt4ProjectManager;

static const Core::Id QT_IS_TEMPORARY("Qt4PM.TempQt");
static const Core::Id PROFILE_IS_TEMPORARY("Qt4PM.TempProfile");
static const Core::Id TEMPORARY_OF_PROJECTS("Qt4PM.TempProject");

TargetSetupPage::TargetSetupPage(QWidget *parent) :
    QWizardPage(parent),
    m_requiredMatcher(0),
    m_preferredMatcher(0),
    m_baseLayout(0),
    m_importSearch(false),
    m_firstWidget(0),
    m_ui(new Internal::Ui::TargetSetupPage),
    m_importWidget(new Internal::ImportWidget),
    m_spacer(new QSpacerItem(0,0, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding))
{
    setObjectName(QLatin1String("TargetSetupPage"));
    m_ui->setupUi(this);

    QWidget *centralWidget = new QWidget(this);
    m_ui->scrollArea->setWidget(centralWidget);
    centralWidget->setLayout(new QVBoxLayout);
    m_ui->centralWidget->setLayout(new QVBoxLayout);
    m_ui->centralWidget->layout()->setMargin(0);

    setUseScrollArea(true);
    setImportSearch(false);

    setTitle(tr("Target Setup"));

    connect(m_ui->descriptionLabel, SIGNAL(linkActivated(QString)),
            this, SIGNAL(noteTextLinkActivated()));

    ProjectExplorer::ProfileManager *sm = ProjectExplorer::ProfileManager::instance();
    connect(sm, SIGNAL(profileAdded(ProjectExplorer::Profile*)),
            this, SLOT(handleProfileAddition(ProjectExplorer::Profile*)));
    connect(sm, SIGNAL(profileRemoved(ProjectExplorer::Profile*)),
            this, SLOT(handleProfileRemoval(ProjectExplorer::Profile*)));
    connect(sm, SIGNAL(profileUpdated(ProjectExplorer::Profile*)),
            this, SLOT(handleProfileUpdate(ProjectExplorer::Profile*)));
    connect(m_importWidget, SIGNAL(importFrom(Utils::FileName)),
            this, SLOT(import(Utils::FileName)));
}

void TargetSetupPage::initializePage()
{
    reset();

    setupWidgets();
    setupImports();
    selectAtLeastOneTarget();
}

void TargetSetupPage::setRequiredProfileMatcher(ProjectExplorer::ProfileMatcher *matcher)
{
    m_requiredMatcher = matcher;
}

void TargetSetupPage::setPreferredProfileMatcher(ProjectExplorer::ProfileMatcher *matcher)
{
    m_preferredMatcher = matcher;
}

TargetSetupPage::~TargetSetupPage()
{
    reset();
    delete m_ui;
    delete m_preferredMatcher;
    delete m_requiredMatcher;
}

bool TargetSetupPage::isProfileSelected(Core::Id id) const
{
    Qt4TargetSetupWidget *widget = m_widgets.value(id);
    return widget && widget->isTargetSelected();
}

void TargetSetupPage::setProfileSelected(Core::Id id, bool selected)
{
    Qt4TargetSetupWidget *widget = m_widgets.value(id);
    if (widget)
        widget->setTargetSelected(selected);
}

bool TargetSetupPage::isQtPlatformSelected(const QString &platform) const
{
    QtSupport::QtPlatformProfileMatcher matcher(platform);
    QList<ProjectExplorer::Profile *> profileList = ProjectExplorer::ProfileManager::instance()->profiles(&matcher);
    foreach (ProjectExplorer::Profile *p, profileList) {
        if (isProfileSelected(p->id()))
            return true;
    }
    return false;
}

bool TargetSetupPage::isComplete() const
{
    foreach (Qt4TargetSetupWidget *widget, m_widgets.values())
        if (widget->isTargetSelected())
            return true;
    return false;
}

void TargetSetupPage::setImportSearch(bool b)
{
    m_importSearch = b;
    m_importWidget->setVisible(b);
}

void TargetSetupPage::setupWidgets()
{
    // Known profiles:
    foreach (ProjectExplorer::Profile *p, ProjectExplorer::ProfileManager::instance()->profiles(m_requiredMatcher))
        addWidget(p);

    // Setup import widget:
    m_baseLayout->addWidget(m_importWidget);
    Utils::FileName path = Utils::FileName::fromString(m_proFilePath);
    path = path.parentDir(); // base dir
    path = path.parentDir(); // parent dir
    m_importWidget->setCurrentDirectory(path);

    updateVisibility();
}

void TargetSetupPage::reset()
{
    foreach (Qt4TargetSetupWidget *widget, m_widgets.values()) {
        ProjectExplorer::Profile *p = widget->profile();
        Q_ASSERT(p);
        if (p->hasValue(PROFILE_IS_TEMPORARY) && !m_proFilePath.isEmpty()) {
            QStringList projects = p->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();
            if (projects.contains(m_proFilePath)) {
                projects.removeOne(m_proFilePath);
                p->setValue(TEMPORARY_OF_PROJECTS, projects);
                if (projects.isEmpty())
                    ProjectExplorer::ProfileManager::instance()->deregisterProfile(p);
            }
        }
        delete widget;
    }

    m_widgets.clear();
    m_firstWidget = 0;
}

void TargetSetupPage::setProFilePath(const QString &path)
{
    m_proFilePath = path;
    if (!m_proFilePath.isEmpty()) {
        m_ui->descriptionLabel->setText(tr("Qt Creator can set up the following targets for project <b>%1</b>:",
                                           "%1: Project name").arg(QFileInfo(m_proFilePath).baseName()));
    }

    reset();
    setupWidgets();
}

void TargetSetupPage::setNoteText(const QString &text)
{
    m_ui->descriptionLabel->setText(text);
}

void TargetSetupPage::import(const Utils::FileName &path)
{
    import(path, false);
}

void TargetSetupPage::import(const Utils::FileName &path, const bool silent)
{
    QFileInfo fi = path.toFileInfo();
    if (!fi.exists() && !fi.isDir())
        return;

    QStringList makefiles = QDir(path.toString()).entryList(QStringList(QLatin1String("Makefile*")));

    QtSupport::BaseQtVersion *version = 0;
    bool temporaryVersion = false;
    ProjectExplorer::Profile *profile = 0;

    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    ProjectExplorer::ProfileManager *pm = ProjectExplorer::ProfileManager::instance();
    bool found = false;

    foreach (const QString &file, makefiles) {
        // find interesting makefiles
        QString makefile = path.toString() + QLatin1Char('/') + file;
        Utils::FileName qmakeBinary = QtSupport::QtVersionManager::findQMakeBinaryFromMakefile(makefile);
        if (qmakeBinary.isEmpty())
            continue;
        if (QtSupport::QtVersionManager::makefileIsFor(makefile, m_proFilePath) != QtSupport::QtVersionManager::SameProject)
            continue;

        // Find version:
        version = vm->qtVersionForQMakeBinary(qmakeBinary);
        if (!version) {
            version = QtSupport::QtVersionFactory::createQtVersionFromQMakePath(qmakeBinary);
            if (!version)
                continue;

            vm->addVersion(version);
            temporaryVersion = true;
        }

        // find qmake arguments and mkspec
        QPair<QtSupport::BaseQtVersion::QmakeBuildConfigs, QString> makefileBuildConfig =
                QtSupport::QtVersionManager::scanMakeFile(makefile, version->defaultBuildConfig());

        QString additionalArguments = makefileBuildConfig.second;
        Utils::FileName parsedSpec =
                Qt4BuildConfiguration::extractSpecFromArguments(&additionalArguments, path.toString(), version);
        Utils::FileName versionSpec = version->mkspec();

        QString specArgument;
        // Compare mkspecs and add to additional arguments
        if (parsedSpec.isEmpty() || parsedSpec == versionSpec
            || parsedSpec == Utils::FileName::fromString(QLatin1String("default"))) {
            // using the default spec, don't modify additional arguments
        } else {
            specArgument = QLatin1String("-spec ") + Utils::QtcProcess::quoteArg(parsedSpec.toUserOutput());
        }
        Utils::QtcProcess::addArgs(&specArgument, additionalArguments);

        // Find profile:
        foreach (ProjectExplorer::Profile *p, pm->profiles()) {
            QtSupport::BaseQtVersion *profileVersion = QtSupport::QtProfileInformation::qtVersion(p);
            Utils::FileName profileSpec = QmakeProfileInformation::mkspec(p);
            if (profileVersion == version
                    && profileSpec == parsedSpec) {
                Q_ASSERT(!temporaryVersion);
                profile = p;
            }
        }
        if (!profile) {
            profile = new ProjectExplorer::Profile;
            profile->setDisplayName(version->displayName());
            QtSupport::QtProfileInformation::setQtVersion(profile, version);
            QmakeProfileInformation::setMkspec(profile, parsedSpec);
            ProjectExplorer::ToolChainProfileInformation::setToolChain(profile,
                                                                       version->preferredToolChain(parsedSpec));

            profile->setValue(PROFILE_IS_TEMPORARY, true);
            profile->setValue(TEMPORARY_OF_PROJECTS, QStringList() << m_proFilePath);
            if (temporaryVersion)
                profile->setValue(QT_IS_TEMPORARY, version->uniqueId());

            pm->registerProfile(profile);
        } else if (profile->hasValue(PROFILE_IS_TEMPORARY)) {
            QStringList profiles = profile->value(TEMPORARY_OF_PROJECTS, QStringList()).toStringList();
            Q_ASSERT(!profiles.contains(m_proFilePath));
            profiles.append(m_proFilePath);
            profile->setValue(PROFILE_IS_TEMPORARY, profiles);
        }

        // Create widget:
        Qt4TargetSetupWidget *widget = m_widgets.value(profile->id(), 0);
        if (!widget)
            addWidget(profile);
        widget = m_widgets.value(profile->id(), 0);
        if (!widget)
            continue;

        // create info:
        BuildConfigurationInfo info = BuildConfigurationInfo(makefileBuildConfig.first,
                                                             specArgument,
                                                             path.toString(),
                                                             true,
                                                             file);

        widget->addBuildConfigurationInfo(info, true);
        widget->setTargetSelected(true);
        found = true;
    }

    updateVisibility();

    if (!found && !silent)
        QMessageBox::critical(this,
                              tr("No Build Found"),
                              tr("No build found in %1 matching project %2.").arg(path.toUserOutput()).arg(m_proFilePath));
}

void TargetSetupPage::handleQtUpdate(const QList<int> &add, const QList<int> &rm, const QList<int> &mod)
{
    Q_UNUSED(add);
    // Update Profile to no longer claim a Qt version is temporary once it is modified/removed.
    foreach (ProjectExplorer::Profile *p, ProjectExplorer::ProfileManager::instance()->profiles()) {
        if (!p->hasValue(QT_IS_TEMPORARY))
            continue;
        int qtVersion = p->value(QT_IS_TEMPORARY, -1).toInt();
        if (rm.contains(qtVersion) || mod.contains(qtVersion))
            p->removeKey(QT_IS_TEMPORARY);
    }
}

void TargetSetupPage::setupImports()
{
    if (!m_importSearch || m_proFilePath.isEmpty())
        return;

    QString sourceDir = QFileInfo(m_proFilePath).absolutePath();
    import(Utils::FileName::fromString(sourceDir), true);

    QList<ProjectExplorer::Profile *> profiles = ProjectExplorer::ProfileManager::instance()->profiles();
    foreach (ProjectExplorer::Profile *p, profiles) {
        QFileInfo fi(Qt4Project::shadowBuildDirectory(m_proFilePath, p, QString()));
        const QString baseDir = fi.absolutePath();
        const QString prefix = fi.baseName();

        foreach (const QString &dir, QDir(baseDir).entryList()) {
            if (dir.startsWith(prefix))
                import(Utils::FileName::fromString(baseDir + QLatin1Char('/') + dir), true);
        }
    }
}

void TargetSetupPage::handleProfileAddition(ProjectExplorer::Profile *p)
{
    Q_ASSERT(!m_widgets.contains(p->id()));
    addWidget(p);
    updateVisibility();
}

void TargetSetupPage::handleProfileRemoval(ProjectExplorer::Profile *p)
{
    QtSupport::QtVersionManager *vm = QtSupport::QtVersionManager::instance();
    QtSupport::BaseQtVersion *version = vm->version(p->value(QT_IS_TEMPORARY, -1).toInt());
    if (version)
        vm->removeVersion(version);

    removeWidget(p);
    updateVisibility();
}

void TargetSetupPage::handleProfileUpdate(ProjectExplorer::Profile *p)
{
    p->removeKey(PROFILE_IS_TEMPORARY);
    p->removeKey(QT_IS_TEMPORARY);
    p->removeKey(TEMPORARY_OF_PROJECTS);

    Qt4TargetSetupWidget *widget = m_widgets.value(p->id());

    bool acceptable = true;
    if (m_requiredMatcher && !m_requiredMatcher->matches(p))
        acceptable = false;

    if (widget && !acceptable)
        removeWidget(p);
    else if (!widget && acceptable)
        addWidget(p);

    updateVisibility();
}

void TargetSetupPage::selectAtLeastOneTarget()
{
    bool atLeastOneTargetSelected = false;
    foreach (Qt4TargetSetupWidget *w, m_widgets.values()) {
        if (w->isTargetSelected()) {
            atLeastOneTargetSelected = true;
            break;
        }
    }

    if (!atLeastOneTargetSelected) {
        Qt4TargetSetupWidget *widget = m_firstWidget;
        ProjectExplorer::Profile *defaultProfile = ProjectExplorer::ProfileManager::instance()->defaultProfile();
        widget = m_widgets.value(defaultProfile->id(), m_firstWidget);
        if (widget)
            widget->setTargetSelected(true);
        m_firstWidget = 0;
    }
    emit completeChanged(); // Is this necessary?
}

void TargetSetupPage::updateVisibility()
{
    if (m_widgets.isEmpty()) {
        // Oh no one can create any targets
        m_ui->scrollAreaWidget->setVisible(false);
        m_ui->centralWidget->setVisible(false);
        m_ui->descriptionLabel->setVisible(false);
        m_ui->noValidProfileLabel->setVisible(true);
    } else {
        m_ui->scrollAreaWidget->setVisible(m_baseLayout == m_ui->scrollArea->widget()->layout());
        m_ui->centralWidget->setVisible(m_baseLayout == m_ui->centralWidget->layout());
        m_ui->descriptionLabel->setVisible(true);
        m_ui->noValidProfileLabel->setVisible(false);
    }

    emit completeChanged();
}

void TargetSetupPage::removeWidget(ProjectExplorer::Profile *p)
{
    Qt4TargetSetupWidget *widget = m_widgets.value(p->id());
    if (!widget)
        return;
    if (widget == m_firstWidget)
        m_firstWidget = 0;
    widget->deleteLater();
    m_widgets.remove(p->id());
}

Qt4TargetSetupWidget *TargetSetupPage::addWidget(ProjectExplorer::Profile *p)
{
    if (m_requiredMatcher && !m_requiredMatcher->matches(p))
        return 0;

    QList<BuildConfigurationInfo> infoList = Qt4BuildConfigurationFactory::availableBuildConfigurations(p, m_proFilePath);
    Qt4TargetSetupWidget *widget = infoList.isEmpty() ? 0 : new Qt4TargetSetupWidget(p, m_proFilePath, infoList);
    if (!widget)
        return 0;

    m_baseLayout->removeWidget(m_importWidget);
    m_baseLayout->removeItem(m_spacer);

    widget->setTargetSelected(m_preferredMatcher && m_preferredMatcher->matches(p));
    m_widgets.insert(p->id(), widget);
    m_baseLayout->addWidget(widget);

    m_baseLayout->addWidget(m_importWidget);
    m_baseLayout->addItem(m_spacer);

    connect(widget, SIGNAL(selectedToggled()),
            this, SIGNAL(completeChanged()));

    if (!m_firstWidget)
        m_firstWidget = widget;

    return widget;
}

bool TargetSetupPage::setupProject(Qt4ProjectManager::Qt4Project *project)
{
    foreach (Qt4TargetSetupWidget *widget, m_widgets.values()) {
        if (!widget->isTargetSelected())
            continue;

        ProjectExplorer::Profile *p = widget->profile();

        // Persist profile:
        p->removeKey(PROFILE_IS_TEMPORARY);
        p->removeKey(QT_IS_TEMPORARY);
        p->removeKey(TEMPORARY_OF_PROJECTS);

        project->addTarget(project->createTarget(p, widget->selectedBuildConfigurationInfoList()));
    }

    // Select active target
    // a) Simulator target
    // b) Desktop target
    // c) the first target
    ProjectExplorer::Target *activeTarget = 0;
    QList<ProjectExplorer::Target *> targets = project->targets();
    foreach (ProjectExplorer::Target *t, targets) {
        QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(t->profile());
        if (version && version->type() == QLatin1String(QtSupport::Constants::SIMULATORQT))
            activeTarget = t;
        else if (!activeTarget && version && version->type() == QLatin1String(QtSupport::Constants::DESKTOPQT))
            activeTarget = t;
    }
    if (!activeTarget && !targets.isEmpty())
        activeTarget = targets.first();
    if (activeTarget)
        project->setActiveTarget(activeTarget);

    return true;
}

void TargetSetupPage::setUseScrollArea(bool b)
{
    m_baseLayout = b ? m_ui->scrollArea->widget()->layout() : m_ui->centralWidget->layout();
    m_ui->scrollAreaWidget->setVisible(b);
    m_ui->centralWidget->setVisible(!b);
}
