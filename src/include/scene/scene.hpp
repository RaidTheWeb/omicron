#ifndef _SCENE__SCENE_HPP
#define _SCENE__SCENE_HPP

#include <scene/gameobject.hpp>
#include <vector>

// Scene file format:
//
// Magic: OSCENE\0
//

namespace OScene {

    class Scene {
        public:
            // description of the actively loaded scene

            // XXX: Collection of high level objects only with children considered below? That approach would resolve inter-object dependancy issues where children rely on the parent to have updated before it does
            std::vector<GameObject *> objects;
            // how should the scene tree be worked out?

            Scene(void) {

            }
    };

}

#endif
