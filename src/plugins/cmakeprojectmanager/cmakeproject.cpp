/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cmakeproject.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectnodes.h"
#include "cmakerunconfiguration.h"
#include "makestep.h"
#include "cmakeopenprojectwizard.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <coreplugin/icore.h>

#include <QtCore/QMap>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QProcess>
#include <QtGui/QFormLayout>
#include <QtGui/QMainWindow>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;


// QtCreator CMake Generator wishlist:
// Which make targets we need to build to get all executables
// What is the make we need to call
// What is the actual compiler executable
// DEFINES

// Open Questions
// Who sets up the environment for cl.exe ? INCLUDEPATH and so on



CMakeProject::CMakeProject(CMakeManager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName),
      m_rootNode(new CMakeProjectNode(m_fileName)),
      m_toolChain(0)
{
    m_file = new CMakeFile(this, fileName);
}

CMakeProject::~CMakeProject()
{
    delete m_rootNode;
    delete m_toolChain;
}

// TODO also call this method if the CMakeLists.txt file changed, which is also called if the CMakeList.txt is updated
// TODO make this function work even if it is reparsing
void CMakeProject::parseCMakeLists()
{
    QString sourceDirectory = QFileInfo(m_fileName).absolutePath();
    QString cbpFile = CMakeManager::findCbpFile(buildDirectory(activeBuildConfiguration()));
    m_rootNode->setFolderName(QFileInfo(cbpFile).baseName());
    CMakeCbpParser cbpparser;
    qDebug()<<"Parsing file "<<cbpFile;
    if (cbpparser.parseCbpFile(cbpFile)) {
        qDebug()<<"CodeBlocks Compilername"<<cbpparser.compilerName();
        ProjectExplorer::ToolChain *newToolChain = 0;
        if (cbpparser.compilerName() == "gcc") {
            newToolChain = ProjectExplorer::ToolChain::createGccToolChain("gcc");
        } else if (cbpparser.compilerName() == "msvc8") {
            // TODO hmm
            //newToolChain = ProjectExplorer::ToolChain::createMSVCToolChain("//TODO");
            Q_ASSERT(false);
        } else {
            // TODO hmm?
            qDebug()<<"Not implemented yet!!! Qt Creator doesn't know which toolchain to use for"<<cbpparser.compilerName();
        }

        if (ProjectExplorer::ToolChain::equals(newToolChain, m_toolChain)) {
            delete newToolChain;
            newToolChain = 0;
        } else {
            delete m_toolChain;
            m_toolChain = newToolChain;
        }

        m_projectName = cbpparser.projectName();
        m_rootNode->setFolderName(cbpparser.projectName());
        qDebug()<<"Building Tree";

        // TODO do a intelligent updating of the tree
        QList<ProjectExplorer::FileNode *> fileList = cbpparser.fileList();
        // Manually add the CMakeLists.txt file
        fileList.append(new ProjectExplorer::FileNode(sourceDirectory + "/CMakeLists.txt", ProjectExplorer::ProjectFileType, false));

        buildTree(m_rootNode, fileList);
        m_files.clear();
        foreach (ProjectExplorer::FileNode *fn, fileList)
            m_files.append(fn->path());
        m_files.sort();

        qDebug()<<"Adding Targets";
        m_targets = cbpparser.targets();
//        qDebug()<<"Printing targets";
//        foreach(CMakeTarget ct, m_targets) {
//            qDebug()<<ct.title<<" with executable:"<<ct.executable;
//            qDebug()<<"WD:"<<ct.workingDirectory;
//            qDebug()<<ct.makeCommand<<ct.makeCleanCommand;
//            qDebug()<<"";
//        }

        qDebug()<<"Updating CodeModel";

        QStringList allIncludePaths;
        QStringList allFrameworkPaths;
        QList<ProjectExplorer::HeaderPath> allHeaderPaths = m_toolChain->systemHeaderPaths();
        foreach (ProjectExplorer::HeaderPath headerPath, allHeaderPaths) {
            if (headerPath.kind() == ProjectExplorer::HeaderPath::FrameworkHeaderPath)
                allFrameworkPaths.append(headerPath.path());
            else
                allIncludePaths.append(headerPath.path());
        }
        allIncludePaths.append(cbpparser.includeFiles());
        CppTools::CppModelManagerInterface *modelmanager = ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>();
        if (modelmanager) {
            CppTools::CppModelManagerInterface::ProjectInfo pinfo = modelmanager->projectInfo(this);
            pinfo.includePaths = allIncludePaths;
            // TODO we only want C++ files, not all other stuff that might be in the project
            pinfo.sourceFiles = m_files;
            pinfo.defines = m_toolChain->predefinedMacros(); // TODO this is to simplistic
            pinfo.frameworkPaths = allFrameworkPaths;
            modelmanager->updateProjectInfo(pinfo);
        }

        // Create run configurations for m_targets
        qDebug()<<"Create run configurations of m_targets";
        QMap<QString, QSharedPointer<CMakeRunConfiguration> > existingRunConfigurations;
        foreach(QSharedPointer<ProjectExplorer::RunConfiguration> cmakeRunConfiguration, runConfigurations()) {
            if (QSharedPointer<CMakeRunConfiguration> rc = cmakeRunConfiguration.dynamicCast<CMakeRunConfiguration>()) {
                existingRunConfigurations.insert(rc->title(), rc);
            }
        }

        bool setActive = false;
        foreach(const CMakeTarget &ct, m_targets) {
            if (ct.executable.isEmpty())
                continue;
            if (ct.title.endsWith("/fast"))
                continue;
            QMap<QString, QSharedPointer<CMakeRunConfiguration> >::iterator it =
                    existingRunConfigurations.find(ct.title);
            if (it != existingRunConfigurations.end()) {
                // Already exists, so override the settings...
                QSharedPointer<CMakeRunConfiguration> rc = it.value();
                qDebug()<<"Updating Run Configuration with title"<<ct.title;
                qDebug()<<"  Executable new:"<<ct.executable<< "old:"<<rc->executable();
                qDebug()<<"  WD new:"<<ct.workingDirectory<<"old:"<<rc->workingDirectory();
                rc->setExecutable(ct.executable);
                rc->setWorkingDirectory(ct.workingDirectory);
                existingRunConfigurations.erase(it);
            } else {
                // Does not exist yet
                qDebug()<<"Adding new run configuration with title"<<ct.title;
                qDebug()<<"  Executable:"<<ct.executable<<"WD:"<<ct.workingDirectory;
                QSharedPointer<ProjectExplorer::RunConfiguration> rc(new CMakeRunConfiguration(this, ct.executable, ct.workingDirectory, ct.title));
                addRunConfiguration(rc);
                // The first one gets the honour of beeing the active one
                if (!setActive) {
                    setActiveRunConfiguration(rc);
                    setActive = true;
                }
            }
        }
        QMap<QString, QSharedPointer<CMakeRunConfiguration> >::const_iterator it =
                existingRunConfigurations.constBegin();
        for( ; it != existingRunConfigurations.constEnd(); ++it) {
            QSharedPointer<CMakeRunConfiguration> rc = it.value();
            qDebug()<<"Removing old RunConfiguration with title:"<<rc->title();
            qDebug()<<"  Executable:"<<rc->executable()<<rc->workingDirectory();
            removeRunConfiguration(rc);
        }
        qDebug()<<"\n";
    } else {
        // TODO report error
        qDebug()<<"Parsing failed";
        delete m_toolChain;
        m_toolChain = 0;
    }
}

QString CMakeProject::buildParser(const QString &buildConfiguration) const
{
    // TODO this is actually slightly wrong, but do i care?
    // this should call toolchain(buildConfiguration)
    if (!m_toolChain)
        return QString::null;
    if (m_toolChain->type() == ProjectExplorer::ToolChain::GCC
        || m_toolChain->type() == ProjectExplorer::ToolChain::LinuxICC
        || m_toolChain->type() == ProjectExplorer::ToolChain::MinGW) {
        return ProjectExplorer::Constants::BUILD_PARSER_GCC;
    } else if (m_toolChain->type() == ProjectExplorer::ToolChain::MSVC
               || m_toolChain->type() == ProjectExplorer::ToolChain::WINCE) {
        return ProjectExplorer::Constants::BUILD_PARSER_MSVC;
    }
    return QString::null;
}

QStringList CMakeProject::targets() const
{
    QStringList results;
    foreach(const CMakeTarget &ct, m_targets)
        results << ct.title;
    return results;
}

void CMakeProject::buildTree(CMakeProjectNode *rootNode, QList<ProjectExplorer::FileNode *> list)
{
    //m_rootNode->addFileNodes(fileList, m_rootNode);
    qSort(list.begin(), list.end(), ProjectExplorer::ProjectNode::sortNodesByPath);
    foreach (ProjectExplorer::FileNode *fn, list) {
        // Get relative path to rootNode
        QString parentDir = QFileInfo(fn->path()).absolutePath();
        ProjectExplorer::FolderNode *folder = findOrCreateFolder(rootNode, parentDir);
        rootNode->addFileNodes(QList<ProjectExplorer::FileNode *>()<< fn, folder);
    }
    //m_rootNode->addFileNodes(list, rootNode);
}

ProjectExplorer::FolderNode *CMakeProject::findOrCreateFolder(CMakeProjectNode *rootNode, QString directory)
{
    QString relativePath = QDir(QFileInfo(rootNode->path()).path()).relativeFilePath(directory);
    QStringList parts = relativePath.split("/", QString::SkipEmptyParts);
    ProjectExplorer::FolderNode *parent = rootNode;
    foreach (const QString &part, parts) {
        // Find folder in subFolders
        bool found = false;
        foreach (ProjectExplorer::FolderNode *folder, parent->subFolderNodes()) {
            if (QFileInfo(folder->path()).fileName() == part) {
                // yeah found something :)
                parent = folder;
                found = true;
                break;
            }
        }
        if (!found) {
            // No FolderNode yet, so create it
            ProjectExplorer::FolderNode *tmp = new ProjectExplorer::FolderNode(part);
            rootNode->addFolderNodes(QList<ProjectExplorer::FolderNode *>() << tmp, parent);
            parent = tmp;
        }
    }
    return parent;
}

QString CMakeProject::name() const
{
    return m_projectName;
}



Core::IFile *CMakeProject::file() const
{
    return m_file;
}

ProjectExplorer::IProjectManager *CMakeProject::projectManager() const
{
    return m_manager;
}

QList<ProjectExplorer::Project *> CMakeProject::dependsOn()
{
    return QList<Project *>();
}

bool CMakeProject::isApplication() const
{
    return true;
}

ProjectExplorer::Environment CMakeProject::environment(const QString &buildConfiguration) const
{
    Q_UNUSED(buildConfiguration)
    //TODO
    return ProjectExplorer::Environment::systemEnvironment();
}

QString CMakeProject::buildDirectory(const QString &buildConfiguration) const
{
    QString buildDirectory = value(buildConfiguration, "buildDirectory").toString();
    if (buildDirectory.isEmpty())
        buildDirectory = QFileInfo(m_fileName).absolutePath() + "/qtcreator-build";
    return buildDirectory;
}

ProjectExplorer::BuildStepConfigWidget *CMakeProject::createConfigWidget()
{
    return new CMakeBuildSettingsWidget(this);
}

QList<ProjectExplorer::BuildStepConfigWidget*> CMakeProject::subConfigWidgets()
{
    return QList<ProjectExplorer::BuildStepConfigWidget*>();
}

// This method is called for new build configurations
// You should probably set some default values in this method
 void CMakeProject::newBuildConfiguration(const QString &buildConfiguration)
 {
     // Default to all
     makeStep()->setBuildTarget(buildConfiguration, "all", true);
 }

ProjectExplorer::ProjectNode *CMakeProject::rootProjectNode() const
{
    return m_rootNode;
}


QStringList CMakeProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode);
    // TODO
    return m_files;
}

void CMakeProject::saveSettingsImpl(ProjectExplorer::PersistentSettingsWriter &writer)
{
    // TODO
    Project::saveSettingsImpl(writer);
}

MakeStep *CMakeProject::makeStep() const
{
    foreach (ProjectExplorer::BuildStep *bs, buildSteps()) {
        MakeStep *ms = qobject_cast<MakeStep *>(bs);
        if (ms)
            return ms;
    }
    return 0;
}


void CMakeProject::restoreSettingsImpl(ProjectExplorer::PersistentSettingsReader &reader)
{
    Project::restoreSettingsImpl(reader);
    bool hasUserFile = !buildConfigurations().isEmpty();
    if (!hasUserFile) {
        // Ask the user for where he wants to build it
        // and the cmake command line

        CMakeOpenProjectWizard copw(m_manager, QFileInfo(m_fileName).absolutePath());
        copw.exec();
        // TODO handle cancel....

        qDebug()<<"ccd.buildDirectory()"<<copw.buildDirectory();

        // Now create a standard build configuration
        MakeStep *makeStep = new MakeStep(this);

        insertBuildStep(0, makeStep);

        addBuildConfiguration("all");
        setActiveBuildConfiguration("all");
        makeStep->setBuildTarget("all", "all", true);
        if (!copw.buildDirectory().isEmpty())
            setValue("all", "buildDirectory", copw.buildDirectory());
        //TODO save arguments somewhere copw.arguments()
    } else {
        // We have a user file, but we could still be missing the cbp file
        // TODO check that we have a cbp file and if not, open up a dialog ?
        // or simply run createXml with the saved settings
        QFileInfo sourceFileInfo(m_fileName);
        QStringList needToCreate;
        QStringList needToUpdate;
        foreach(const QString &buildConfiguration, buildConfigurations()) {
            QString buildDirectory = value(buildConfiguration, "buildDirectory").toString();
            QString cbpFile = CMakeManager::findCbpFile(QDir(buildDirectory));
            QFileInfo cbpFileFi(cbpFile);
            if (!cbpFileFi.exists())
                needToCreate << buildDirectory;
            else if (cbpFileFi.lastModified() < sourceFileInfo.lastModified())
                needToUpdate << buildDirectory;
        }
        if (!needToCreate.isEmpty() || !needToUpdate.isEmpty()) {
            CMakeOpenProjectWizard copw(m_manager, sourceFileInfo.absolutePath(), needToCreate, needToUpdate);
            copw.exec();
        }
    }

    parseCMakeLists(); // Gets the directory from the active buildconfiguration
}

CMakeFile::CMakeFile(CMakeProject *parent, QString fileName)
    : Core::IFile(parent), m_project(parent), m_fileName(fileName)
{

}

bool CMakeFile::save(const QString &fileName)
{
    // TODO
    // Once we have an texteditor open for this file, we probably do
    // need to implement this, don't we.
    Q_UNUSED(fileName);
    return false;
}

QString CMakeFile::fileName() const
{
    return m_fileName;
}

QString CMakeFile::defaultPath() const
{
    return QString();
}

QString CMakeFile::suggestedFileName() const
{
    return QString();
}

QString CMakeFile::mimeType() const
{
    return Constants::CMAKEMIMETYPE;
}


bool CMakeFile::isModified() const
{
    return false;
}

bool CMakeFile::isReadOnly() const
{
    return true;
}

bool CMakeFile::isSaveAsAllowed() const
{
    return false;
}

void CMakeFile::modified(ReloadBehavior *behavior)
{
    Q_UNUSED(behavior);
}

CMakeBuildSettingsWidget::CMakeBuildSettingsWidget(CMakeProject *project)
    : m_project(project)
{
    QFormLayout *fl = new QFormLayout(this);
    setLayout(fl);
    m_pathChooser = new Core::Utils::PathChooser(this);
    m_pathChooser->setEnabled(false);
    // TODO currently doesn't work
    // since creating the cbp file also creates makefiles
    // and then cmake builds in that directory instead of shadow building
    // We need our own generator for that to work
    connect(m_pathChooser, SIGNAL(changed()), this, SLOT(buildDirectoryChanged()));
    fl->addRow("Build directory:", m_pathChooser);
}

QString CMakeBuildSettingsWidget::displayName() const
{
    return "CMake";
}

void CMakeBuildSettingsWidget::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;
    m_pathChooser->setPath(m_project->buildDirectory(buildConfiguration));
}

void CMakeBuildSettingsWidget::buildDirectoryChanged()
{
    m_project->setValue(m_buildConfiguration, "buildDirectory", m_pathChooser->path());
}

/////
// CMakeCbpParser
////

bool CMakeCbpParser::parseCbpFile(const QString &fileName)
{
    QFile fi(fileName);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        setDevice(&fi);

        while (!atEnd()) {
            readNext();
            if (name() == "CodeBlocks_project_file") {
                parseCodeBlocks_project_file();
            } else if (isStartElement()) {
                parseUnknownElement();
            }
        }
        fi.close();
        m_includeFiles.sort();
        m_includeFiles.removeDuplicates();
        return true;
    }
    return false;
}

void CMakeCbpParser::parseCodeBlocks_project_file()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "Project") {
            parseProject();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseProject()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "Option") {
            parseOption();
        } else if (name() == "Unit") {
            parseUnit();
        } else if (name() == "Build") {
            parseBuild();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseBuild()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "Target") {
            parseTarget();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseTarget()
{
    m_targetType = false;
    m_target.clear();

    if (attributes().hasAttribute("title"))
        m_target.title = attributes().value("title").toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            if (m_targetType || m_target.title == "all" || m_target.title == "install") {
                m_targets.append(m_target);
            }
            return;
        } else if (name() == "Compiler") {
            parseCompiler();
        } else if (name() == "Option") {
            parseTargetOption();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseTargetOption()
{
    if (attributes().hasAttribute("output"))
        m_target.executable = attributes().value("output").toString();
    else if (attributes().hasAttribute("type") && (attributes().value("type") == "1" || attributes().value("type") == "0"))
        m_targetType = true;
    else if (attributes().hasAttribute("working_dir"))
        m_target.workingDirectory = attributes().value("working_dir").toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "MakeCommand") {
            parseMakeCommand();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

QString CMakeCbpParser::projectName() const
{
    return m_projectName;
}

void CMakeCbpParser::parseOption()
{
    if (attributes().hasAttribute("title"))
        m_projectName = attributes().value("title").toString();

    if (attributes().hasAttribute("compiler"))
        m_compiler = attributes().value("compiler").toString();

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if(isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseMakeCommand()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "Build") {
            parseTargetBuild();
        } else if (name() == "Clean") {
            parseTargetClean();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseTargetBuild()
{
    if (attributes().hasAttribute("command"))
        m_target.makeCommand = attributes().value("command").toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseTargetClean()
{
    if (attributes().hasAttribute("command"))
        m_target.makeCleanCommand = attributes().value("command").toString();
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseCompiler()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (name() == "Add") {
            parseAdd();
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseAdd()
{
    m_includeFiles.append(attributes().value("directory").toString());
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseUnit()
{
    //qDebug()<<stream.attributes().value("filename");
    QString fileName = attributes().value("filename").toString();
    if (!fileName.endsWith(".rule"))
        m_fileList.append( new ProjectExplorer::FileNode(fileName, ProjectExplorer::SourceType, false));
    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            return;
        } else if (isStartElement()) {
            parseUnknownElement();
        }
    }
}

void CMakeCbpParser::parseUnknownElement()
{
    Q_ASSERT(isStartElement());

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            parseUnknownElement();
    }
}

QList<ProjectExplorer::FileNode *> CMakeCbpParser::fileList()
{
    return m_fileList;
}

QStringList CMakeCbpParser::includeFiles()
{
    return m_includeFiles;
}

QList<CMakeTarget> CMakeCbpParser::targets()
{
    return m_targets;
}

QString CMakeCbpParser::compilerName() const
{
    return m_compiler;
}

void CMakeTarget::clear()
{
    executable = QString::null;
    makeCommand = QString::null;
    makeCleanCommand = QString::null;
    workingDirectory = QString::null;
    title = QString::null;
}

