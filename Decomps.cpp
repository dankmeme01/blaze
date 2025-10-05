// This file does not contain code compiled into the mod, rather it contains some decompiled snippets.
// Code here is not going to be 1 to 1 with the actual GD code, however the behavior should be identical.
// This is here just for preservation reasons, as the actual functions will be rewritten for optimization purposes.


void GJBaseGameLayer::processRotationActionsReimpl() {
    float sx = m_objectLayer->getScaleX();
    float sy = m_objectLayer->getScaleY();
    auto pos = m_objectLayer->getPosition();

    m_objectLayer->setScale(1.f);
    m_objectLayer->setPosition({0.f, 0.f});

    for (auto& pair : m_effectManager->m_unkMap5c8) {
        for (auto obj : pair.second) {
            obj->m_unkInt204 = m_gameState.m_unkUint2;
        }
    }

    for (auto& obj : m_effectManager->m_unkVector5b0) {
        if (obj->m_unkInt204 != m_gameState.m_unkUint2 || obj->m_someInterpValue1RelatedFalse) {
            continue;
        }

        auto mainObj = this->tryGetMainObject(obj->m_centerGroupID);
        auto sgroup = this->getStaticGroup(obj->m_targetGroupID);
        auto ogroup = this->getOptimizedGroup(obj->m_targetGroupID);

        bool v17;
        double v18;

        if (sgroup->data->num) {
            v17 = false;
            ogroup = this->getGroup(obj->m_targetGroupID);
            v18 = obj->m_someInterpValue1RelatedOne - obj->m_someInterpValue1RelatedZero;
        } else {
            v17 = true;
            v18 = obj->m_someInterpValue2RelatedOne - obj->m_someInterpValue2RelatedZero;
        }
        float v46 = v18;

        bool finishRelated = obj->m_finishRelated;
        if (v18 == 0.0 && !finishRelated) {
            continue;
        }

        obj->m_someInterpValue1RelatedFalse = true;
        if (obj->m_lockObjectRotation) {
            v18 = 0.0;
        }

        float v47 = v18;
        this->claimRotationAction(obj->m_targetGroupID, obj->m_centerGroupID, v46, v47, v17, true);

        if (mainObj) {
            auto pos = mainObj->getUnmodifiedPosition();
            auto claimed = this->claimMoveAction(obj->m_targetGroupID, v17);

            m_areaTransformNode->setPosition(pos);
            m_areaTransformNode->setScaleX(1.0f);
            m_areaTransformNode->setScaleY(1.0f);
            m_areaScaleNode->setScaleX(1.0f);
            m_areaScaleNode->setScaleY(1.0f);
            m_areaTransformNode->setSkewX(0.f);
            m_areaTransformNode->setSkewY(0.f);
            m_areaSkewNode->setSkewX(0.f);
            m_areaSkewNode->setSkewY(0.f);
            m_areaTransformNode->setRotation(v46);
            this->prepareTransformParent(false);
            auto t1 = m_areaTransformNode->nodeToWorldTransform();
            auto t2 = m_areaSkewNode->nodeToWorldTransform();
            auto t3 = m_areaScaleNode->nodeToWorldTransform();
            m_areaTransformNode2->setScaleX(1.0f);
            m_areaTransformNode2->setScaleY(1.0f);
            m_areaTransformNode2->setSkewX(0.f);
            m_areaTransformNode2->setSkewY(0.f);
            m_rotatedCount += ogroup->data->num;

            for (auto obj : CCArrayExt<GameObject>(ogroup)) {
                obj->m_unk4fb = finishRelated;

                if (!obj->m_isDecoration2) {
                    if (obj->m_unk4C4 != m_gameState.m_unkUint2) {
                        obj->m_lastPosition.x = obj->m_positionX;
                        obj->m_lastPosition.y = obj->m_positionY;
                        obj->m_unk4C4 = m_gameState.m_unkUint2;
                        obj->dirtifyObjectRect();
                    }
                }

                float xo = obj->m_positionXOffset;
                float yo = obj->m_positionYOffset;
                obj->m_isDirty = true;
                obj->m_isUnmodifiedPosDirty = true;
                float v32 = obj->m_positionX - xo - pos.x;
                float v33 = obj->m_positionY - yo - pos.y;
                auto tresult = __CCPointApplyAffineTransform({v32, v33}, t1);

                obj->m_positionX = obj->m_positionXOffset + tresult.x + claimed.x;
                obj->m_positionY = obj->m_positionYOffset + tresult.y + claimed.y;

                if (v47 != 0.0 && obj->m_canRotateFree) {
                    obj->m_rotationXOffset += v47;
                    obj->m_rotationYOffset += v47;
                    obj->addRotation(v47);
                    if (obj->m_objectType != GameObjectType::Decoration && !obj->m_shouldUseOuterOb) {
                        obj->calculateOrientedBox();
                    }
                }
                this->updateObjectSection(obj);
            }

            this->updateDisabledObjectsLastPos(ogroup);
        } else if (ogroup && v47 != 0.0 && ogroup->data->num) {
            m_rotatedCount += ogroup->data->num;

            for (auto obj : CCArrayExt<GameObject>(ogroup)) {
                if (obj->m_canRotateFree) {
                    obj->m_rotationXOffset += v47;
                    obj->m_rotationYOffset += v47;
                    obj->addRotation(v47);
                    if (obj->m_objectType != GameObjectType::Decoration) {
                        obj->m_isObjectRectDirty = true;
                        obj->m_isOrientedBoxDirty = true;
                        if (!obj->m_shouldUseOuterOb) {
                            obj->calculateOrientedBox();
                        }
                    }
                }
            }
        }
    }

    m_objectLayer->setScaleX(sx);
    m_objectLayer->setScaleY(sy);
    m_objectLayer->setPosition(pos);
}

void GJBaseGameLayer::processMoveActionsStep(float p0, bool p1) {
    m_unked0 = 0;
    m_disabledObjectsCount = 0;
    m_unked8 = 0;
    m_areaObjectsCount = 0;

    for (auto& act : m_gameState.m_dynamicObjActions1) {
        int gid = static_cast<EffectGameObject*>(act.m_gameObject1)->m_targetGroupID;
        auto command = m_effectManager->tryGetMoveCommandNode(gid);
        if (command) command->m_unk0d1 = true;
    }

    this->processDynamicObjectActions(1, p0);
    this->processTransformActions(p1);
    this->processRotationActionsReimpl();
    this->processDynamicObjectActions(0, p0);
    this->processMoveActions();
    this->processPlayerFollowActions(p0);
    this->processAdvancedFollowActions(p0);
    this->processFollowActions();
    this->processAreaActions(p0, p1);

    if (m_isEditor && m_disabledObjectsCount > 0) {
        for (int i = 0; i < m_disabledObjectsCount; i++) {
            auto obj = m_disabledObjects[i];
            obj->quickUpdatePosition();
        }
    }
}

void GJBaseGameLayer::processMoveActions() {
    auto& vec = m_effectManager->m_unkVector708;

    // inline processMoveCalculations
    for (auto& action : vec) {
        float val = -action.m_gameObject->m_rotationXOffset * M_PI / 180.f;
        float cosine = cosf(val);
        float sine = sinf(val);
        float offx = action.m_offset.x;
        float offy = action.m_offset.y;

        CCPoint point;
        point.x = (offx * cosine - offy * sine) * (action.m_gameObject->m_scaleXOffset + 1.0f);
        point.y = (offy * cosine + offx * sine) * (action.m_gameObject->m_scaleYOffset + 1.0f);
        point -= action.m_offset;

        auto mn = action.m_moveNode;
        mn->m_unk0d1 = false;
        mn->m_unk038 += point.x;
        mn->m_unk040 += point.y;
        mn->m_unk090 += point.x;
        mn->m_unk098 += point.y;
    }
    vec.clear();

    for (auto action : m_effectManager->m_unkVector6c0) {
        if (action->m_unk0d0) continue;

        int tag = action->getTag();
        auto sgroup = this->getStaticGroup(tag);
        if (sgroup && (action->m_unk038 != 0.0 || action->m_unk040 != 0.0 || action->m_unk078)) {
            this->moveObjectsReimpl(sgroup, action->m_unk038, action->m_unk040, action->m_eObjType == CCObjectType::GameObject);
        }

        sgroup = this->getOptimizedGroup(tag);
        if (sgroup && (action->m_unk090 != 0.0 || action->m_unk098 != 0.0 || action->m_unk078)) {
            this->moveObjectsReimpl(sgroup, action->m_unk090, action->m_unk098, action->m_eObjType == CCObjectType::GameObject);
        }
    }
}

void GJBaseGameLayer::moveObjects(CCArray* arr, double offsetx, double offsety, bool p3) {
    int count = arr->count();
    if (count == 0) return;

    // log::debug("Moving {} objects with p1 = {}, p2 = {}, p3 = {}", count, offsetx, offsety, p3);

    m_movedCount += count;

    for (auto obj : CCArrayExt<GameObject>(arr)) {
        if (!obj->m_isDecoration2 && obj->m_unk4C4 != m_gameState.m_unkUint2) {
            obj->m_lastPosition.x = obj->m_positionX;
            obj->m_lastPosition.y = obj->m_positionY;
            obj->m_unk4C4 = m_gameState.m_unkUint2;
            obj->dirtifyObjectRect();
        }

        if (offsetx != 0.0 && !obj->m_tempOffsetXRelated) {
            obj->m_positionX += offsetx;
        }

        if (offsety != 0.0) {
            obj->m_positionY += offsety;
        }

        obj->dirtifyObjectPos();
        this->updateObjectSection(obj);
    }

    this->updateDisabledObjectsLastPos(arr);
}

void GJBaseGameLayer::updateObjectSection(GameObject* obj) {
    if (obj->m_outerSectionIndex < 0) return;

    double posx = obj->m_positionX;
    if (posx <= 0.0) {
        posx = 0.0;
    } else {
        if (posx < 1000000.0) {
            posx = (double)m_sectionXFactor * posx;
        } else {
            posx = (double)m_sectionXFactor * 1000000.0;
        }
    }

    double posy = obj->m_positionY;
    if (posy <= 0.0) {
        posy = 0.0;
    } else {
        if (posy < 1000000.0) {
            posy = (double)m_sectionYFactor * posy;
        } else {
            posy = (double)m_sectionYFactor * 1000000.0;
        }
    }

    if ((int)posx != obj->m_outerSectionIndex || (int)posy != obj->m_middleSectionIndex) {
        // inline reorderObjectSection (2 calls)
        this->removeObjectFromSection(obj);
        this->addToSection(obj);

        if (obj->m_isActivated) {
            int outer = obj->m_outerSectionIndex;
            int middle = obj->m_middleSectionIndex;

            if (outer > m_rightSectionIndex || outer < m_leftSectionIndex || middle < m_bottomSectionIndex || middle > m_topSectionIndex) {
                m_objectsToDeactivate->setObject(obj, obj->m_uniqueID);
                obj->m_unk3ee = true;
            }
        }
    }
}
