/*
 * Copyright � 2020 Vsevolod Kremianskii
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

#pragma once

#include <list>

#include "../../gui/types.h"
#include "../../net/types.h"
#include "../../render/scene/scenegraph.h"
#include "../../render/types.h"
#include "../../resources/types.h"
#include "../../script/variable.h"

#include "../actions.h"
#include "../camera/firstperson.h"
#include "../camera/thirdperson.h"
#include "../collisiondetect.h"
#include "../objectselect.h"
#include "../pathfinder.h"
#include "../room.h"

#include "creature.h"
#include "door.h"
#include "placeable.h"
#include "trigger.h"
#include "waypoint.h"

namespace reone {

namespace game {

typedef std::unordered_map<std::string, std::shared_ptr<Room>> RoomMap;
typedef std::vector<std::shared_ptr<SpatialObject>> ObjectList;

class ObjectFactory;

class Area : public Object {
public:
    Area(
        uint32_t id,
        resources::GameVersion version,
        ObjectFactory *objectFactory,
        render::SceneGraph *sceneGraph,
        const render::GraphicsOptions &opts);

    void load(const std::string &name, const resources::GffStruct &are, const resources::GffStruct &git);
    void loadParty(const PartyConfiguration &party, const glm::vec3 &position, float heading);
    void loadCameras(const glm::vec3 &entryPosition, float entryHeading);
    void runOnEnterScript();

    bool handle(const SDL_Event &event);
    void update(const UpdateContext &updateCtx);

    void fill(const UpdateContext &updateCtx, GuiContext &guiCtx);
    bool moveCreatureTowards(Creature &creature, const glm::vec2 &dest, bool run, float dt);
    void updateTriggers(const Creature &creature);
    void updateRoomVisibility();
    SpatialObject *getObjectAt(int x, int y) const;

    void update3rdPersonCameraTarget();
    void update3rdPersonCameraHeading();
    void switchTo3rdPersonCamera();
    void toggleCameraType();
    Camera *getCamera() const;
    void startDialog(Creature &creature, const std::string &resRef);

    void delayCommand(uint32_t timestamp, const script::ExecutionContext &ctx);
    int eventUserDefined(int eventNumber);
    void signalEvent(int eventId);

    std::shared_ptr<SpatialObject> find(uint32_t id) const;
    std::shared_ptr<SpatialObject> find(const std::string &tag, int nth = 0) const;

    // Load/save
    void saveTo(GameState &state) const;
    void loadState(const GameState &state);

    // General getters
    const CameraStyle &cameraStyle() const;
    CameraType cameraType() const;
    const std::string &music() const;
    const RoomMap &rooms() const;
    const ObjectList &objects() const;
    const CollisionDetector &collisionDetector() const;
    const Pathfinder &pathfinder() const;
    ThirdPersonCamera *thirdPersonCamera();
    ObjectSelector &objectSelector();

    // Party getters
    std::shared_ptr<SpatialObject> player() const;
    std::shared_ptr<SpatialObject> partyLeader() const;
    std::shared_ptr<SpatialObject> partyMember1() const;
    std::shared_ptr<SpatialObject> partyMember2() const;

    // Callbacks
    void setOnCameraChanged(const std::function<void(CameraType)> &fn);
    void setOnModuleTransition(const std::function<void(const std::string &, const std::string &)> &fn);
    void setOnPlayerChanged(const std::function<void()> &fn);
    void setOnStartDialog(const std::function<void(const Object &, const std::string &)> &fn);

protected:
    ObjectFactory *_objectFactory { nullptr };
    render::SceneGraph *_sceneGraph { nullptr };
    bool _scriptsEnabled { true };
    float _cameraAspect { 0.0f };
    CameraType _cameraType { CameraType::FirstPerson };
    std::unique_ptr<FirstPersonCamera> _firstPersonCamera;
    std::unique_ptr<ThirdPersonCamera> _thirdPersonCamera;
    std::function<void()> _onPlayerChanged;

    ObjectList _objects;
    std::unordered_map<ObjectType, ObjectList> _objectsByType;
    std::unordered_map<uint32_t, std::shared_ptr<SpatialObject>> _objectById;
    std::unordered_map<std::string, ObjectList> _objectsByTag;

    // Party
    std::shared_ptr<SpatialObject> _player;
    std::shared_ptr<SpatialObject> _partyLeader;
    std::shared_ptr<SpatialObject> _partyMember1;
    std::shared_ptr<SpatialObject> _partyMember2;

    virtual void add(const std::shared_ptr<SpatialObject> &object);
    void determineObjectRoom(SpatialObject &object);
    void landObject(SpatialObject &object);
    virtual void updateCreature(Creature &creature, float dt);

private:
    enum class ScriptType {
        OnEnter,
        OnExit,
        OnHeartbeat,
        OnUserDefined
    };

    struct DelayedCommand {
        uint32_t timestamp { 0 };
        script::ExecutionContext context;
        bool executed { false };
    };

    struct UserDefinedEvent {
        int eventNumber { 0 };
    };

    resources::GameVersion _version { resources::GameVersion::KotOR };
    render::GraphicsOptions _opts;
    CollisionDetector _collisionDetector;
    Pathfinder _pathfinder;
    ObjectSelector _objectSelector;
    ActionExecutor _actionExecutor;
    std::string _name;
    RoomMap _rooms;
    std::unique_ptr<resources::Visibility> _visibility;
    CameraStyle _cameraStyle;
    std::string _music;
    std::unordered_map<ScriptType, std::string> _scripts;
    std::list<DelayedCommand> _delayed;
    std::map<int, UserDefinedEvent> _events;
    int _eventCounter { 0 };

    // Callbacks
    std::function<void(const std::string &, const std::string &)> _onModuleTransition;
    std::function<void(const Object &, const std::string &)> _onStartDialog;
    std::function<void(CameraType)> _onCameraChanged;

    std::shared_ptr<Creature> makeCharacter(const CreatureConfiguration &character, const std::string &tag, const glm::vec3 &position, float heading);
    void updateDelayedCommands();
    bool getElevationAt(const glm::vec2 &position, Room *&room, float &z) const;
    void addPartyMemberPortrait(const std::shared_ptr<SpatialObject> &object, GuiContext &ctx);
    void addDebugInfo(const UpdateContext &updateCtx, GuiContext &guiCtx);
    glm::vec3 getSelectableScreenCoords(uint32_t objectId, const UpdateContext &ctx) const;

    // Loading

    void loadLYT();
    void loadVIS();
    void loadPTH();
    void loadARE(const resources::GffStruct &are);
    void loadCameraStyle(const resources::GffStruct &are);
    void loadAmbientColor(const resources::GffStruct &are);
    void loadScripts(const resources::GffStruct &are);
    void loadGIT(const resources::GffStruct &gffs);
    void loadProperties(const resources::GffStruct &git);
    void loadCreatures(const resources::GffStruct &git);
    void loadDoors(const resources::GffStruct &git);
    void loadPlaceables(const resources::GffStruct &git);
    void loadWaypoints(const resources::GffStruct &git);
    void loadTriggers(const resources::GffStruct &git);

    // END Loading

    // Events

    bool handleKeyDown(const SDL_KeyboardEvent &event);

    // END Events
};

} // namespace game

} // namespace reone