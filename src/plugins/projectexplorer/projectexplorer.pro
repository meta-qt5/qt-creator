TEMPLATE = lib
TARGET = ProjectExplorer
QT += xml script network
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += quick1
} else {
    QT += declarative
}

include(../../qtcreatorplugin.pri)
include(projectexplorer_dependencies.pri)
include(../../libs/utils/utils.pri)
include(../../libs/ssh/ssh.pri)
include(customwizard/customwizard.pri)
INCLUDEPATH += $$PWD/../../libs/utils
HEADERS += projectexplorer.h \
    abi.h \
    abiwidget.h \
    clangparser.h \
    gcctoolchain.h \
    projectexplorer_export.h \
    projectwindow.h \
    profile.h \
    profileconfigwidget.h \
    profileinformation.h \
    profileinformationconfigwidget.h \
    profilemanager.h \
    profilemanagerconfigwidget.h \
    profilemodel.h \
    profileoptionspage.h \
    buildmanager.h \
    buildsteplist.h \
    compileoutputwindow.h \
    deployconfiguration.h \
    deployconfigurationmodel.h \
    namedwidget.h \
    target.h \
    targetsettingspanel.h \
    task.h \
    itaskhandler.h \
    copytaskhandler.h \
    showineditortaskhandler.h \
    showoutputtaskhandler.h \
    vcsannotatetaskhandler.h \
    taskwindow.h \
    taskmodel.h \
    projectfilewizardextension.h \
    session.h \
    dependenciespanel.h \
    allprojectsfilter.h \
    ioutputparser.h \
    projectconfiguration.h \
    gnumakeparser.h \
    projectexplorerconstants.h \
    projectexplorersettings.h \
    corelistenercheckingforrunningbuild.h \
    project.h \
    pluginfilefactory.h \
    iprojectmanager.h \
    currentprojectfilter.h \
    allprojectsfind.h \
    buildstep.h \
    buildconfiguration.h \
    iprojectproperties.h \
    buildsettingspropertiespage.h \
    environmentwidget.h \
    processstep.h \
    editorconfiguration.h \
    editorsettingspropertiespage.h \
    runconfiguration.h \
    applicationlauncher.h \
    applicationrunconfiguration.h \
    runsettingspropertiespage.h \
    projecttreewidget.h \
    foldernavigationwidget.h \
    buildprogress.h \
    projectnodes.h \
    sessiondialog.h \
    projectwizardpage.h \
    buildstepspage.h \
    removefiledialog.h \
    nodesvisitor.h \
    projectmodels.h \
    currentprojectfind.h \
    toolchain.h \
    toolchainconfigwidget.h \
    toolchainmanager.h \
    toolchainoptionspage.h \
    cesdkhandler.h \
    gccparser.h \
    projectexplorersettingspage.h \
    projectwelcomepage.h \
    baseprojectwizarddialog.h \
    miniprojecttargetselector.h \
    targetselector.h \
    targetsettingswidget.h \
    doubletabwidget.h \
    buildenvironmentwidget.h \
    ldparser.h \
    linuxiccparser.h \
    runconfigurationmodel.h \
    buildconfigurationmodel.h \
    processparameters.h \
    abstractprocessstep.h \
    taskhub.h \
    localapplicationruncontrol.h \
    metatypedeclarations.h \
    publishing/publishingwizardselectiondialog.h \
    publishing/ipublishingwizardfactory.h \
    headerpath.h \
    gcctoolchainfactories.h \
    appoutputpane.h \
    codestylesettingspropertiespage.h \
    settingsaccessor.h \
    environmentitemswidget.h \
    devicesupport/idevice.h \
    devicesupport/desktopdevice.h \
    devicesupport/desktopdevicefactory.h \
    devicesupport/idevicewidget.h \
    devicesupport/idevicefactory.h \
    devicesupport/devicemanager.h \
    devicesupport/devicemanagermodel.h \
    devicesupport/devicefactoryselectiondialog.h \
    devicesupport/devicesettingswidget.h \
    devicesupport/devicesettingspage.h

SOURCES += projectexplorer.cpp \
    abi.cpp \
    abiwidget.cpp \
    clangparser.cpp \
    gcctoolchain.cpp \
    projectwindow.cpp \
    profile.cpp \
    profileinformation.cpp \
    profileinformationconfigwidget.cpp \
    profilemanager.cpp \
    profilemanagerconfigwidget.cpp \
    profilemodel.cpp \
    profileoptionspage.cpp \
    buildmanager.cpp \
    buildsteplist.cpp \
    compileoutputwindow.cpp \
    deployconfiguration.cpp \
    deployconfigurationmodel.cpp \
    namedwidget.cpp \
    target.cpp \
    targetsettingspanel.cpp \
    ioutputparser.cpp \
    projectconfiguration.cpp \
    gnumakeparser.cpp \
    task.cpp \
    copytaskhandler.cpp \
    showineditortaskhandler.cpp \
    showoutputtaskhandler.cpp \
    vcsannotatetaskhandler.cpp \
    taskwindow.cpp \
    taskmodel.cpp \
    projectfilewizardextension.cpp \
    session.cpp \
    dependenciespanel.cpp \
    allprojectsfilter.cpp \
    currentprojectfilter.cpp \
    allprojectsfind.cpp \
    project.cpp \
    pluginfilefactory.cpp \
    buildstep.cpp \
    buildconfiguration.cpp \
    buildsettingspropertiespage.cpp \
    environmentwidget.cpp \
    processstep.cpp \
    abstractprocessstep.cpp \
    editorconfiguration.cpp \
    editorsettingspropertiespage.cpp \
    runconfiguration.cpp \
    applicationlauncher.cpp \
    applicationrunconfiguration.cpp \
    runsettingspropertiespage.cpp \
    projecttreewidget.cpp \
    foldernavigationwidget.cpp \
    buildprogress.cpp \
    projectnodes.cpp \
    sessiondialog.cpp \
    projectwizardpage.cpp \
    buildstepspage.cpp \
    removefiledialog.cpp \
    nodesvisitor.cpp \
    projectmodels.cpp \
    currentprojectfind.cpp \
    toolchain.cpp \
    toolchainconfigwidget.cpp \
    toolchainmanager.cpp \
    toolchainoptionspage.cpp \
    cesdkhandler.cpp \
    gccparser.cpp \
    projectexplorersettingspage.cpp \
    projectwelcomepage.cpp \
    corelistenercheckingforrunningbuild.cpp \
    baseprojectwizarddialog.cpp \
    miniprojecttargetselector.cpp \
    targetselector.cpp \
    targetsettingswidget.cpp \
    doubletabwidget.cpp \
    buildenvironmentwidget.cpp \
    ldparser.cpp \
    linuxiccparser.cpp \
    runconfigurationmodel.cpp \
    buildconfigurationmodel.cpp \
    taskhub.cpp \
    processparameters.cpp \
    localapplicationruncontrol.cpp \
    publishing/publishingwizardselectiondialog.cpp \
    appoutputpane.cpp \
    codestylesettingspropertiespage.cpp \
    settingsaccessor.cpp \
    environmentitemswidget.cpp \
    devicesupport/idevice.cpp \
    devicesupport/desktopdevice.cpp \
    devicesupport/desktopdevicefactory.cpp \
    devicesupport/idevicefactory.cpp \
    devicesupport/devicemanager.cpp \
    devicesupport/devicemanagermodel.cpp \
    devicesupport/devicefactoryselectiondialog.cpp \
    devicesupport/devicesettingswidget.cpp \
    devicesupport/devicesettingspage.cpp

FORMS += processstep.ui \
    editorsettingspropertiespage.ui \
    sessiondialog.ui \
    projectwizardpage.ui \
    removefiledialog.ui \
    projectexplorersettingspage.ui \
    targetsettingswidget.ui \
    doubletabwidget.ui \
    publishing/publishingwizardselectiondialog.ui \
    codestylesettingspropertiespage.ui \
    devicesupport/devicefactoryselectiondialog.ui \
    devicesupport/devicesettingswidget.ui

WINSOURCES += \
    windebuginterface.cpp \
    msvcparser.cpp \
    msvctoolchain.cpp \
    abstractmsvctoolchain.cpp \
    wincetoolchain.cpp

WINHEADERS += \
    windebuginterface.h \
    msvcparser.h \
    msvctoolchain.h \
    abstractmsvctoolchain.h \
    wincetoolchain.h

win32|equals(TEST, 1) {
    SOURCES += $$WINSOURCES
    HEADERS += $$WINHEADERS
}

equals(TEST, 1) {
    SOURCES += \
        outputparser_test.cpp
    HEADERS += \
        outputparser_test.h
}

macx:LIBS += -framework Carbon

RESOURCES += projectexplorer.qrc

# Some way to override the architecture used in Abi:
!isEmpty($$(QTC_CPU)) {
    DEFINES += QTC_CPU=$$(QTC_CPU)
} else {
    DEFINES += QTC_CPU=X86Architecture
}

DEFINES += PROJECTEXPLORER_LIBRARY
