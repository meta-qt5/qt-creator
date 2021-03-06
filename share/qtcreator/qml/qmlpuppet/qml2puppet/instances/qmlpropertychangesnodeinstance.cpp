/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlpropertychangesnodeinstance.h"
#include "qmlstatenodeinstance.h"

#include <qmlprivategate.h>

#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QMutableListIterator>

namespace QmlDesigner {
namespace Internal {

QmlPropertyChangesNodeInstance::QmlPropertyChangesNodeInstance(QObject *propertyChangesObject) :
        ObjectNodeInstance(propertyChangesObject)
{
}

QmlPropertyChangesNodeInstance::Pointer QmlPropertyChangesNodeInstance::create(QObject *object)
{
    Pointer instance(new QmlPropertyChangesNodeInstance(object));

    instance->populateResetHashes();

    return instance;
}

void QmlPropertyChangesNodeInstance::setPropertyVariant(const PropertyName &name, const QVariant &value)
{
    if (QmlPrivateGate::PropertyChanges::isNormalProperty(name)) { // 'restoreEntryValues', 'explicit'
        ObjectNodeInstance::setPropertyVariant(name, value);
    } else {
        QmlPrivateGate::PropertyChanges::changeValue(object(), name, value);
        QObject *targetObject = QmlPrivateGate::PropertyChanges::targetObject(object());
        if (targetObject
                && nodeInstanceServer()->activeStateInstance().
                isWrappingThisObject(QmlPrivateGate::PropertyChanges::stateObject(object()))) {
            if (nodeInstanceServer()->hasInstanceForObject(targetObject)) {
                ServerNodeInstance targetInstance = nodeInstanceServer()->instanceForObject(targetObject);
                targetInstance.setPropertyVariant(name, value);
            }
        }
    }
}

void QmlPropertyChangesNodeInstance::setPropertyBinding(const PropertyName &name, const QString &expression)
{
    if (QmlPrivateGate::PropertyChanges::isNormalProperty(name)) { // 'restoreEntryValues', 'explicit'
        ObjectNodeInstance::setPropertyBinding(name, expression);
    } else {
        QmlPrivateGate::PropertyChanges::changeExpression(object(), name, expression);
    }
}

QVariant QmlPropertyChangesNodeInstance::property(const PropertyName &name) const
{
    return QmlPrivateGate::PropertyChanges::getProperty(object(), name);
}

void QmlPropertyChangesNodeInstance::resetProperty(const PropertyName &name)
{
    QmlPrivateGate::PropertyChanges::removeProperty(object(), name);
}


void QmlPropertyChangesNodeInstance::reparent(const ObjectNodeInstance::Pointer &oldParentInstance, const PropertyName &oldParentProperty, const ObjectNodeInstance::Pointer &newParentInstance, const PropertyName &newParentProperty)
{
    QmlPrivateGate::PropertyChanges::detachFromState(object());

    ObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);

    QmlPrivateGate::PropertyChanges::attachToState(object());
}

} // namespace Internal
} // namespace QmlDesigner
