/****************************************************************************
**
** Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "calcboundingvolumejob_p.h"

#include <Qt3DCore/qboundingvolume.h>
#include <Qt3DCore/qabstractfrontendnodemanager.h>
#include <Qt3DCore/private/qgeometry_p.h>
#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/entity_p.h>
#include <Qt3DRender/private/renderlogging_p.h>
#include <Qt3DRender/private/managers_p.h>
#include <Qt3DRender/private/qgeometryrenderer_p.h>
#include <Qt3DRender/private/geometryrenderer_p.h>
#include <Qt3DRender/private/geometry_p.h>
#include <Qt3DRender/private/buffermanager_p.h>
#include <Qt3DRender/private/attribute_p.h>
#include <Qt3DRender/private/buffer_p.h>
#include <Qt3DRender/private/sphere_p.h>
#include <Qt3DRender/private/buffervisitor_p.h>
#include <Qt3DRender/private/entityvisitor_p.h>

#include <QtCore/qmath.h>
#if QT_CONFIG(concurrent)
#include <QtConcurrent/QtConcurrent>
#endif
#include <Qt3DRender/private/job_common_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {
namespace Render {

namespace {

class BoundingVolumeCalculator
{
public:
    BoundingVolumeCalculator(NodeManagers *manager) : m_manager(manager) { }

    const Sphere& result() { return m_volume; }
    const QVector3D min() const { return m_min; }
    const QVector3D max() const { return m_max; }

    bool apply(Qt3DRender::Render::Attribute *positionAttribute,
               Qt3DRender::Render::Attribute *indexAttribute,
               int drawVertexCount,
               bool primitiveRestartEnabled,
               int primitiveRestartIndex)
    {
        FindExtremePoints findExtremePoints(m_manager);
        if (!findExtremePoints.apply(positionAttribute, indexAttribute, drawVertexCount,
                                     primitiveRestartEnabled, primitiveRestartIndex))
            return false;

        m_min = QVector3D(findExtremePoints.xMin, findExtremePoints.yMin, findExtremePoints.zMin);
        m_max = QVector3D(findExtremePoints.xMax, findExtremePoints.yMax, findExtremePoints.zMax);

        FindMaxDistantPoint maxDistantPointY(m_manager);
        maxDistantPointY.setReferencePoint = true;
        if (!maxDistantPointY.apply(positionAttribute, indexAttribute, drawVertexCount,
                                     primitiveRestartEnabled, primitiveRestartIndex))
            return false;
        if (maxDistantPointY.hasNoPoints)
            return false;

        //const Vector3D x = maxDistantPointY.referencePt;
        const Vector3D y = maxDistantPointY.maxDistPt;

        FindMaxDistantPoint maxDistantPointZ(m_manager);
        maxDistantPointZ.setReferencePoint = false;
        maxDistantPointZ.referencePt = y;
        if (!maxDistantPointZ.apply(positionAttribute, indexAttribute, drawVertexCount,
                                     primitiveRestartEnabled, primitiveRestartIndex)) {
            return false;
        }
        const Vector3D z = maxDistantPointZ.maxDistPt;

        const Vector3D center = (y + z) * 0.5f;

        FindMaxDistantPoint maxDistantPointCenter(m_manager);
        maxDistantPointCenter.setReferencePoint = false;
        maxDistantPointCenter.referencePt = center;
        if (!maxDistantPointCenter.apply(positionAttribute, indexAttribute, drawVertexCount,
                                     primitiveRestartEnabled, primitiveRestartIndex)) {
            return false;
        }

        const float radius = (center - maxDistantPointCenter.maxDistPt).length();

        m_volume = Qt3DRender::Render::Sphere(center, radius);

        if (m_volume.isNull())
            return false;

        return true;
    }

private:
    Sphere m_volume;
    NodeManagers *m_manager;
    QVector3D m_min;
    QVector3D m_max;

    class FindExtremePoints : public Buffer3fVisitor
    {
    public:
        FindExtremePoints(NodeManagers *manager)
            : Buffer3fVisitor(manager)
            , xMin(0.0f), xMax(0.0f), yMin(0.0f), yMax(0.0f), zMin(0.0f), zMax(0.0f)
        { }

        float xMin, xMax, yMin, yMax, zMin, zMax;
        Vector3D xMinPt, xMaxPt, yMinPt, yMaxPt, zMinPt, zMaxPt;

        void visit(uint ndx, float x, float y, float z) override
        {
            if (ndx) {
                if (x < xMin) {
                    xMin = x;
                    xMinPt = Vector3D(x, y, z);
                }
                if (x > xMax) {
                    xMax = x;
                    xMaxPt = Vector3D(x, y, z);
                }
                if (y < yMin) {
                    yMin = y;
                    yMinPt = Vector3D(x, y, z);
                }
                if (y > yMax) {
                    yMax = y;
                    yMaxPt = Vector3D(x, y, z);
                }
                if (z < zMin) {
                    zMin = z;
                    zMinPt = Vector3D(x, y, z);
                }
                if (z > zMax) {
                    zMax = z;
                    zMaxPt = Vector3D(x, y, z);
                }
            } else {
                xMin = xMax = x;
                yMin = yMax = y;
                zMin = zMax = z;
                xMinPt = xMaxPt = yMinPt = yMaxPt = zMinPt = zMaxPt = Vector3D(x, y, z);
            }
        }
    };

    class FindMaxDistantPoint : public Buffer3fVisitor
    {
    public:
        FindMaxDistantPoint(NodeManagers *manager)
            : Buffer3fVisitor(manager)
        { }

        float maxLengthSquared = 0.0f;
        Vector3D maxDistPt;
        Vector3D referencePt;
        bool setReferencePoint = false;
        bool hasNoPoints = true;

        void visit(uint ndx, float x, float y, float z) override
        {
            Q_UNUSED(ndx);
            const Vector3D p = Vector3D(x, y, z);

            if (hasNoPoints && setReferencePoint) {
                maxLengthSquared = 0.0f;
                referencePt = p;
            }
            const float lengthSquared = (p - referencePt).lengthSquared();
            if ( lengthSquared >= maxLengthSquared ) {
                maxDistPt = p;
                maxLengthSquared = lengthSquared;
            }
            hasNoPoints = false;
        }
    };
};

struct BoundingVolumeComputeData {
    Entity *entity = nullptr;
    GeometryRenderer *renderer = nullptr;
    Geometry *geometry = nullptr;
    Attribute *positionAttribute = nullptr;
    Attribute *indexAttribute = nullptr;
    int vertexCount = -1;

    bool valid() const { return vertexCount >= 0; }
};

BoundingVolumeComputeData findBoundingVolumeComputeData(NodeManagers *manager, Entity *node)
{
    BoundingVolumeComputeData res;
    res.entity = node;

    res.renderer = node->renderComponent<GeometryRenderer>();
    if (!res.renderer || res.renderer->primitiveType() == QGeometryRenderer::Patches)
        return res;

    GeometryManager *geometryManager = manager->geometryManager();
    res.geometry = geometryManager->lookupResource(res.renderer->geometryId());
    if (!res.geometry)
        return res;

    if (res.renderer->hasView())
        return res;

    int drawVertexCount = res.renderer->vertexCount(); // may be 0, gets changed below if so

    Qt3DRender::Render::Attribute *positionAttribute = manager->lookupResource<Attribute, AttributeManager>(res.geometry->boundingPositionAttribute());

    // Use the default position attribute if attribute is null
    if (!positionAttribute) {
        const auto attrIds = res.geometry->attributes();
        for (const Qt3DCore::QNodeId &attrId : attrIds) {
            positionAttribute = manager->lookupResource<Attribute, AttributeManager>(attrId);
            if (positionAttribute &&
                positionAttribute->name() == QAttribute::defaultPositionAttributeName())
                break;
        }
    }

    if (!positionAttribute
        || positionAttribute->attributeType() != QAttribute::VertexAttribute
        || positionAttribute->vertexBaseType() != QAttribute::Float
        || positionAttribute->vertexSize() < 3) {
        qWarning("findBoundingVolumeComputeData: Position attribute not suited for bounding volume computation");
        return res;
    }

    Buffer *buf = manager->lookupResource<Buffer, BufferManager>(positionAttribute->bufferId());
    // No point in continuing if the positionAttribute doesn't have a suitable buffer
    if (!buf) {
        qWarning("findBoundingVolumeComputeData: Position attribute not referencing a valid buffer");
        return res;
    }

    // Check if there is an index attribute.
    Qt3DRender::Render::Attribute *indexAttribute = nullptr;
    Buffer *indexBuf = nullptr;
    const QVector<Qt3DCore::QNodeId> attributes = res.geometry->attributes();

    for (Qt3DCore::QNodeId attrNodeId : attributes) {
        Qt3DRender::Render::Attribute *attr = manager->lookupResource<Attribute, AttributeManager>(attrNodeId);
        if (attr && attr->attributeType() == QAttribute::IndexAttribute) {
            indexBuf = manager->lookupResource<Buffer, BufferManager>(attr->bufferId());
            if (indexBuf) {
                indexAttribute = attr;

                if (!drawVertexCount)
                    drawVertexCount = indexAttribute->count();

                const QAttribute::VertexBaseType validIndexTypes[] = {
                    QAttribute::UnsignedShort,
                    QAttribute::UnsignedInt,
                    QAttribute::UnsignedByte
                };

                if (std::find(std::begin(validIndexTypes),
                              std::end(validIndexTypes),
                              indexAttribute->vertexBaseType()) == std::end(validIndexTypes)) {
                    qWarning() << "findBoundingVolumeComputeData: Unsupported index attribute type" << indexAttribute->name() << indexAttribute->vertexBaseType();
                    return res;
                }

                break;
            }
        }
    }

    if (!indexAttribute && !drawVertexCount)
        drawVertexCount = positionAttribute->count();

    // Buf will be set to not dirty once it's loaded
    // in a job executed after this one
    // We need to recompute the bounding volume
    // If anything in the GeometryRenderer has changed
    if (buf->isDirty()
        || node->isBoundingVolumeDirty()
        || positionAttribute->isDirty()
        || res.geometry->isDirty()
        || res.renderer->isDirty()
        || (indexAttribute && indexAttribute->isDirty())
        || (indexBuf && indexBuf->isDirty())) {
        res.vertexCount = drawVertexCount;
        res.positionAttribute = positionAttribute;
        res.indexAttribute = indexAttribute;
    }

    return res;
}

QVector<Geometry *> calculateLocalBoundingVolume(NodeManagers *manager, const BoundingVolumeComputeData &data)
{
    // The Bounding volume will only be computed if the position Buffer
    // isDirty

    QVector<Geometry *> updatedGeometries;

    BoundingVolumeCalculator reader(manager);
    if (reader.apply(data.positionAttribute, data.indexAttribute, data.vertexCount,
                     data.renderer->primitiveRestartEnabled(), data.renderer->restartIndexValue())) {
        data.entity->localBoundingVolume()->setCenter(reader.result().center());
        data.entity->localBoundingVolume()->setRadius(reader.result().radius());
        data.entity->unsetBoundingVolumeDirty();

        // Record min/max vertex in Geometry
        data.geometry->updateExtent(reader.min(), reader.max());
        // Mark geometry as requiring a call to update its frontend
        updatedGeometries.push_back(data.geometry);
    }

    return updatedGeometries;
}

struct UpdateBoundFunctor
{
    NodeManagers *manager;

    // This define is required to work with QtConcurrent
    typedef QVector<Geometry *> result_type;
    QVector<Geometry *> operator ()(const BoundingVolumeComputeData &data)
    {
        return calculateLocalBoundingVolume(manager, data);
    }
};

struct ReduceUpdateBoundFunctor
{
    void operator ()(QVector<Geometry *> &result, const QVector<Geometry *> &values)
    {
        result += values;
    }
};

class DirtyEntityAccumulator : public EntityVisitor
{
public:
    DirtyEntityAccumulator(NodeManagers *manager)
        : EntityVisitor(manager)
    {
    }

    EntityVisitor::Operation visit(Entity *entity) override
    {
        if (!entity->isTreeEnabled())
            return Prune;
        auto data = findBoundingVolumeComputeData(m_manager, entity);

        if (data.valid()) {
            // only valid if front end is a QGeometryRenderer without a view. All other cases handled by core aspect
            m_entities.push_back(data);
        } else {
            if (!data.renderer || data.renderer->primitiveType() == QGeometryRenderer::Patches
                || !data.renderer->hasView())    // should have been handled above
                return Continue;

            // renderer has a view, we can pull the data from the front end
            QBoundingVolume *frontEndBV = qobject_cast<QBoundingVolume *>(m_frontEndNodeManager->lookupNode(data.renderer->peerId()));
            if (!frontEndBV)
                return Continue;
            auto dFrontEndBV = QGeometryRendererPrivate::get(frontEndBV);

            // copy data to the entity
            if (dFrontEndBV->m_explicitPointsValid) {
                const auto diagonal = dFrontEndBV->m_maxPoint - dFrontEndBV->m_minPoint;
                entity->localBoundingVolume()->setCenter(Vector3D(dFrontEndBV->m_minPoint + diagonal * .5f));
                entity->localBoundingVolume()->setRadius(diagonal.length() * .5f);
            } else {
                entity->localBoundingVolume()->setCenter(Vector3D(dFrontEndBV->m_implicitCenter));
                entity->localBoundingVolume()->setRadius(dFrontEndBV->m_implicitRadius);
                entity->unsetBoundingVolumeDirty();
                // copy the data to the geometry
                data.geometry->updateExtent(dFrontEndBV->m_implicitMinPoint, dFrontEndBV->m_implicitMaxPoint);
            }
        }

        return Continue;
    }

    NodeManagers *m_nodeNanager;
    Qt3DCore::QAbstractFrontEndNodeManager *m_frontEndNodeManager = nullptr;
    std::vector<BoundingVolumeComputeData> m_entities;
};

} // anonymous


CalculateBoundingVolumeJob::CalculateBoundingVolumeJob()
    : Qt3DCore::QAspectJob()
    , m_manager(nullptr)
    , m_node(nullptr)
    , m_frontEndNodeManager(nullptr)
{
    SET_JOB_RUN_STAT_TYPE(this, JobTypes::CalcBoundingVolume, 0)
}

void CalculateBoundingVolumeJob::run()
{
    Q_ASSERT(m_frontEndNodeManager);

    DirtyEntityAccumulator accumulator(m_manager);
    accumulator.m_frontEndNodeManager = m_frontEndNodeManager;
    accumulator.m_nodeNanager = m_manager;
    accumulator.apply(m_node);

    std::vector<BoundingVolumeComputeData> entities = std::move(accumulator.m_entities);

    QVector<Geometry *> updatedGeometries;
    updatedGeometries.reserve(entities.size());

#if QT_CONFIG(concurrent)
    if (entities.size() > 1) {
        UpdateBoundFunctor functor;
        functor.manager = m_manager;
        ReduceUpdateBoundFunctor reduceFunctor;
        updatedGeometries += QtConcurrent::blockingMappedReduced<decltype(updatedGeometries)>(entities, functor, reduceFunctor);
    } else
#endif
    {
        for (const auto &data: entities)
            updatedGeometries += calculateLocalBoundingVolume(m_manager, data);
    }

    m_updatedGeometries = std::move(updatedGeometries);
}

void CalculateBoundingVolumeJob::postFrame(QAspectEngine *aspectEngine)
{
    Q_UNUSED(aspectEngine)
    for (Geometry *backend : qAsConst(m_updatedGeometries)) {
        Qt3DCore::QGeometry *node = qobject_cast<Qt3DCore::QGeometry *>(m_frontEndNodeManager->lookupNode(backend->peerId()));
        if (!node)
            continue;
        Qt3DCore::QGeometryPrivate *dNode = static_cast<Qt3DCore::QGeometryPrivate *>(Qt3DCore::QNodePrivate::get(node));
        dNode->setExtent(backend->min(), backend->max());
    }

    m_updatedGeometries.clear();
}

void CalculateBoundingVolumeJob::setRoot(Entity *node)
{
    m_node = node;
}

void CalculateBoundingVolumeJob::setManagers(NodeManagers *manager)
{
    m_manager = manager;
}

void CalculateBoundingVolumeJob::setFrontEndNodeManager(Qt3DCore::QAbstractFrontEndNodeManager *manager)
{
    m_frontEndNodeManager = manager;
}

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE

