/*
 * Copyright (c) 2020 The reone project contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stack>
#include <stdexcept>

#include "../../common/log.h"
#include "../../render/mesh/aabb.h"
#include "../../resource/resources.h"

#include "../scenegraph.h"

#include "aabbnode.h"
#include "lightnode.h"
#include "modelnodescenenode.h"
#include "modelscenenode.h"

using namespace std;

using namespace reone::render;
using namespace reone::resource;

namespace reone {

namespace scene {

ModelSceneNode::ModelSceneNode(SceneGraph *sceneGraph, const shared_ptr<Model> &model, const set<string> &skipNodes) :
    SceneNode(sceneGraph),
    _model(model),
    _animator(this, skipNodes) {

    initModelNodes();
}

void ModelSceneNode::initModelNodes() {
    shared_ptr<ModelNodeSceneNode> rootNode(getModelNodeSceneNode(_model->rootNode()));
    addChild(rootNode);

    stack<ModelNodeSceneNode *> nodes;
    nodes.push(rootNode.get());

    while (!nodes.empty()) {
        ModelNodeSceneNode *sceneNode = nodes.top();
        nodes.pop();

        ModelNode *modelNode = sceneNode->modelNode();
        _modelNodeByIndex.insert(make_pair(modelNode->index(), sceneNode));
        _modelNodeByNumber.insert(make_pair(modelNode->nodeNumber(), sceneNode));

        for (auto &child : modelNode->children()) {
            shared_ptr<ModelNodeSceneNode> childNode(getModelNodeSceneNode(*child));
            addChild(childNode);
            nodes.push(childNode.get());

            shared_ptr<ModelNode::Light> light(child->light());
            if (light) {
                shared_ptr<LightSceneNode> lightNode(new LightSceneNode(_sceneGraph, light->priority, child->color(), child->radius(), child->multiplier(), light->shadow));
                childNode->addChild(lightNode);
            }
        }
    }
}

unique_ptr<ModelNodeSceneNode> ModelSceneNode::getModelNodeSceneNode(ModelNode &node) const {
    unique_ptr<ModelNodeSceneNode> sceneNode(new ModelNodeSceneNode(_sceneGraph, this, &node));
    sceneNode->setLocalTransform(node.absoluteTransform());
    return move(sceneNode);
}

void ModelSceneNode::update(float dt) {
    if (!_visible || !_onScreen) return;

    _animator.update(dt);

    for (auto &attached : _attachedModels) {
        attached.second->update(dt);
    }
}

void ModelSceneNode::render() const {
}

void ModelSceneNode::playDefaultAnimation() {
    _animator.playDefaultAnimation();

    for (auto &attached : _attachedModels) {
        attached.second->playDefaultAnimation();
    }
}

void ModelSceneNode::playAnimation(const string &name, int flags, float speed) {
    _animator.playAnimation(name, flags, speed);

    if (flags & kAnimationPropagate) {
        for (auto &attached : _attachedModels) {
            attached.second->playAnimation(name, flags, speed);
        }
    }
}

shared_ptr<ModelSceneNode> ModelSceneNode::attach(const string &parent, const shared_ptr<Model> &model) {
    ModelNodeSceneNode *parentNode = getModelNode(parent);
    if (!parentNode) return nullptr;

    ModelNode *parentModelNode = parentNode->modelNode();
    uint16_t parentNumber = parentModelNode->nodeNumber();

    auto maybeAttached = _attachedModels.find(parentNumber);
    if (maybeAttached != _attachedModels.end()) {
        parentNode->removeChild(*maybeAttached->second);
        _attachedModels.erase(maybeAttached);
    }
    if (model) {
        set<string> skipNodes;
        for (const ModelNode *node = parentModelNode; node; node = node->parent()) {
            skipNodes.insert(node->name());
        }
        shared_ptr<ModelSceneNode> modelNode(new ModelSceneNode(_sceneGraph, model, skipNodes));
        modelNode->setLightingEnabled(_lightingEnabled && model->classification() != Model::Classification::Lightsaber);
        parentNode->addChild(modelNode);

        auto inserted = _attachedModels.insert(make_pair(parentNumber, move(modelNode)));
        return inserted.first->second;
    }

    return nullptr;
}

ModelNodeSceneNode *ModelSceneNode::getModelNode(const string &name) const {
    shared_ptr<ModelNode> modelNode(_model->findNodeByName(name));
    if (!modelNode) return nullptr;

    auto maybeSceneNode = _modelNodeByNumber.find(modelNode->nodeNumber());
    if (maybeSceneNode == _modelNodeByNumber.end()) return nullptr;

    return maybeSceneNode->second;
}

ModelNodeSceneNode *ModelSceneNode::getModelNodeByIndex(int index) const {
    auto maybeNode = _modelNodeByIndex.find(index);
    if (maybeNode == _modelNodeByIndex.end()) return nullptr;

    return maybeNode->second;
}

shared_ptr<ModelSceneNode> ModelSceneNode::getAttachedModel(const string &parent) const {
    shared_ptr<ModelNode> parentModelNode(_model->findNodeByName(parent));
    if (!parentModelNode) {
        warn("ModelSceneNode: parent node not found: " + parent);
        return nullptr;
    }
    auto maybeAttached = _attachedModels.find(parentModelNode->nodeNumber());
    return maybeAttached != _attachedModels.end() ? maybeAttached->second : nullptr;
}

void ModelSceneNode::attach(const string &parent, const shared_ptr<SceneNode> &node) {
    shared_ptr<ModelNode> parentModelNode(_model->findNodeByName(parent));
    if (!parentModelNode) {
        warn("ModelSceneNode: parent node not found: " + parent);
        return;
    }
    uint16_t parentNumber = parentModelNode->nodeNumber();

    auto maybeNode = _modelNodeByNumber.find(parentNumber);
    if (maybeNode != _modelNodeByNumber.end()) {
        maybeNode->second->addChild(node);
    }
}

void ModelSceneNode::updateAbsoluteTransform() {
    SceneNode::updateAbsoluteTransform();
    _lightingDirty = true;
}

void ModelSceneNode::updateLighting() {
    if (!_lightingEnabled || !_lightingDirty) return;

    _lightsAffectedBy.clear();
    glm::vec3 center(_absoluteTransform * glm::vec4(_model->aabb().center(), 1.0f));

    _sceneGraph->getLightsAt(center, _lightsAffectedBy);
    _lightingDirty = false;

    for (auto &attached : _attachedModels) {
        attached.second->setLightsAffectedBy(_lightsAffectedBy);
    }
}

void ModelSceneNode::setLightingIsDirty() {
    _lightingDirty = true;
}

bool ModelSceneNode::getNodeAbsolutePosition(const string &name, glm::vec3 &position) const {
    shared_ptr<ModelNode> node(_model->findNodeByName(name));
    if (!node) {
        shared_ptr<Model> superModel(_model->superModel());
        if (superModel) {
            node = superModel->findNodeByName(name);
        }
    }
    if (!node) return false;

    position = node->absoluteTransform()[3];

    return true;
}

glm::vec3 ModelSceneNode::getCenterOfAABB() const {
    return _absoluteTransform * glm::vec4(_model->aabb().center(), 1.0f);
}

const string &ModelSceneNode::name() const {
    return _model->name();
}

shared_ptr<Model> ModelSceneNode::model() const {
    return _model;
}

bool ModelSceneNode::hasTextureOverride() const {
    return static_cast<bool>(_textureOverride);
}

shared_ptr<Texture> ModelSceneNode::textureOverride() const {
    return _textureOverride;
}

bool ModelSceneNode::isVisible() const {
    return _visible;
}

bool ModelSceneNode::isOnScreen() const {
    return _onScreen;
}

float ModelSceneNode::alpha() const {
    return _alpha;
}

const AABB &ModelSceneNode::aabb() const {
    return _model->aabb();
}

bool ModelSceneNode::isLightingEnabled() const {
    return _lightingEnabled;
}

const vector<LightSceneNode *> &ModelSceneNode::lightsAffectedBy() const {
    return _lightsAffectedBy;
}

void ModelSceneNode::setTextureOverride(const shared_ptr<Texture> &texture) {
    _textureOverride = texture;
}

void ModelSceneNode::setVisible(bool visible) {
    if (_visible == visible) return;

    _visible = visible;

    for (auto &attached : _attachedModels) {
        attached.second->setVisible(visible);
    }
    if (visible) {
        _lightingDirty = true;
    }
}

void ModelSceneNode::setOnScreen(bool onScreen) {
    _onScreen = onScreen;

    for (auto &attached : _attachedModels) {
        attached.second->setOnScreen(onScreen);
    }
}

void ModelSceneNode::setAlpha(float alpha) {
    _alpha = alpha;

    for (auto &attached : _attachedModels) {
        attached.second->setAlpha(alpha);
    }
}

bool ModelSceneNode::isAnimationFinished() const {
    return _model ? _animator.isAnimationFinished() : false;
}

void ModelSceneNode::setDefaultAnimation(const string &name) {
    _animator.setDefaultAnimation(name);

    for (auto &attached : _attachedModels) {
        attached.second->setDefaultAnimation(name);
    }
}

void ModelSceneNode::setLightingEnabled(bool enabled) {
    _lightingEnabled = enabled;
}

void ModelSceneNode::setLightsAffectedBy(const vector<LightSceneNode *> &lights) {
    _lightsAffectedBy = lights;
}

} // namespace scene

} // namespace reone
