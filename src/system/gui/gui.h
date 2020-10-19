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

#include <memory>
#include <unordered_map>
#include <vector>

#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"

#include "../resource/gfffile.h"
#include "../resource/types.h"

#include "control/control.h"

namespace reone {

namespace gui {

static const int kDefaultResolutionX = 640;
static const int kDefaultResolutionY = 480;

class GUI {
public:
    virtual void load();

    virtual bool handle(const SDL_Event &event);
    virtual void update(float dt);
    virtual void render() const;
    virtual void render3D() const;

    void resetFocus();

    void configureRootContol(const std::function<void(Control &)> &fn);
    void configureControl(const std::string &tag, const std::function<void(Control &)> &fn);
    void showControl(const std::string &tag);
    void hideControl(const std::string &tag);
    void enableControl(const std::string &tag);
    void disableControl(const std::string &tag);

    void setControlFocusable(const std::string &tag, bool focusable);
    void setControlDisabled(const std::string &tag, bool disabled);
    void setControlText(const std::string &tag, const std::string &text);
    void setControlFocus(const std::string &tag, bool focus);
    void setControlHilightColor(const std::string &tag, const glm::vec3 &color);

    Control &getControl(const std::string &tag) const;

protected:
    enum class ScalingMode {
        Center,
        PositionRelativeToCenter,
        Stretch
    };

    resource::GameVersion _version { resource::GameVersion::KotOR };
    render::GraphicsOptions _gfxOpts;
    std::string _resRef;
    BackgroundType _backgroundType { BackgroundType::None };
    int _resolutionX { kDefaultResolutionX };
    int _resolutionY { kDefaultResolutionY };
    ScalingMode _scaling { ScalingMode::Center };
    float _aspect { 0.0f };
    glm::ivec2 _screenCenter { 0 };
    glm::ivec2 _rootOffset { 0 };
    glm::ivec2 _controlOffset { 0 };
    std::shared_ptr<render::Texture> _background;
    std::unique_ptr<Control> _rootControl;
    std::vector<std::unique_ptr<Control>> _controls;
    std::unordered_map<std::string, Control *> _controlByTag;
    Control *_focus { nullptr };

    GUI(resource::GameVersion version, const render::GraphicsOptions &opts);

    std::string getResRef(const std::string &base) const;
    void loadBackground(BackgroundType type);
    void loadControl(const resource::GffStruct &gffs);
    virtual void preloadControl(Control &control);

    virtual bool handleKeyDown(SDL_Scancode key);
    virtual bool handleKeyUp(SDL_Scancode key);

    virtual void onFocusChanged(const std::string &control, bool focus);
    virtual void onClick(const std::string &control);

private:
    GUI(const GUI &) = delete;
    GUI &operator=(const GUI &) = delete;

    void positionRelativeToCenter(Control &control);
    void stretchControl(Control &control);
    void updateFocus(int x, int y);
    Control *getControlAt(int x, int y, const std::function<bool(const Control &)> &test) const;
    void drawBackground() const;
};

} // namespace gui

} // namespace reone