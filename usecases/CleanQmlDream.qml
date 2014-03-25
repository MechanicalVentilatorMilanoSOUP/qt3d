/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.1 as QQ2
import Qt3D 2.0

Node {
    // Node can contains any type of Node or QObject
    id : root
    objectName : "root"


    // Entity can be composed of components and child entities
    // than inherit its attributes (transformations, framegraph rendering)
    Entity {
        id : sceneRoot
        objectName : sceneRoot

        FrameGraph {
            SampleFrameGraphConfiguration {
                viewportRect : Qt.rect(0,0,1,1)
                camera : camera1 // Problem here, we cannot reference an Entity
            }
        }

        // Cameras are Entity composed of an Optic component called Camera
        // Maybe Camera should be its own Entity subclass
        Entity {
            id : camera1
            objectName : "camera1"

            Transform {
                LookAt {}
                Translate {}
                Scale {}
                Rotate {}
            }

            Camera { // Maybe this should be rename to optics
                projectionType: Camera.PerspectiveProjection
                fieldOfView: 22.5
                aspectRatio: 1920.0 / 1080.0
            }
        }

        Entity {
            id : testMesh
            objectName : "testMesh"

            Mesh mesh : Mesh { // subclass of Component
                source : "test_mesh.obj"

                Material { // subclass of Component
                    diffuseColor : "#00ff00"
                    specularColor : "#00ff00"
                }
            }
        }

    } // sceneRoot

} // root
