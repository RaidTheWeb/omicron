#include <engine/scene/gameobject.hpp>
#include <engine/scene/scene.hpp>

namespace OScene {

    glm::mat4 GameObject::getmatrix(void) {
        if (this->dirty.load()) {
            this->matrix = glm::translate(this->position) * glm::toMat4(this->orientation) * glm::scale(this->scale);
            this->dirty.store(false);
        }

        return this->matrix;
    }

    glm::mat4 GameObject::getglobalmatrix(void) {
        return this->parent.isvalid() ? this->parent->getglobalmatrix() * this->getmatrix() : this->getmatrix();
    }

    glm::vec3 GameObject::getglobalposition(void) {
        return this->parent.isvalid() ? this->parent->getglobalposition() + this->position : this->position;
    }

    glm::vec3 GameObject::getposition(void) {
        return this->position;
    }

    glm::quat GameObject::getglobalorientation(void) {
        return this->parent.isvalid() ? glm::normalize(this->parent->getglobalorientation() * this->orientation) : this->orientation;
    }

    glm::quat GameObject::getorientation(void) {
        return this->orientation;
    }

    glm::vec3 GameObject::getglobalscale(void) {
        return this->parent.isvalid() ? this->parent->getglobalscale() * this->scale : this->scale;
    }

    glm::vec3 GameObject::getscale(void) {
        return this->scale;
    }

    void GameObject::setglobalposition(glm::vec3 pos) {
        ASSERT(this->scene != NULL, "Cannot set global position without a scene.\n");
        this->position = pos - (this->getglobalposition() - this->position); // Transform into a local position (Subtracting our parent's global position will turn the other global position into a local position, we use the parent's global position instead of our's because we want it to be *relative* to the parent, not *relative* to the current position)
        this->scene->partitionmanager.updatepos(this->gethandle());
        this->dirty.store(true);
    }

    void GameObject::setposition(glm::vec3 pos) {
        ASSERT(this->scene != NULL, "Cannot set position without a scene.\n");
        this->position = pos;
        this->scene->partitionmanager.updatepos(this->gethandle());
        this->dirty.store(true);
    }

    void GameObject::setorientation(glm::quat q) {
        this->orientation = q;
        this->dirty.store(true);
    }

    void GameObject::setrotation(glm::vec3 euler) {
        this->orientation = glm::quat(euler);
        this->dirty.store(true);
    }

    void GameObject::setscale(glm::vec3 scale) {
        this->scale = scale;
        this->dirty.store(true);
    }

    void GameObject::translate(glm::vec3 v) {
        ASSERT(this->scene != NULL, "Cannot translate without a scene.\n");
        this->position += v;
        this->scene->partitionmanager.updatepos(this->gethandle());
        this->dirty.store(true);
    }

    void GameObject::orientate(glm::quat q) {
        this->orientation = glm::normalize(this->orientation * q);
        this->dirty.store(true);
    }

    void GameObject::lookat(glm::vec4 target) {
        this->orientation = glm::quatLookAt(-glm::normalize(target.w ? glm::vec3(target) - this->position : glm::vec3(target)), glm::vec3(0.0f, 1.0f, 0.0f));
        this->dirty.store(true);
    }

    void GameObject::scaleby(glm::vec3 s) {
        this->scale *= s;
        this->dirty.store(true);
    }
    


    GameObjectAllocator objallocator;
    std::atomic<size_t> objidcounter = 1;
    OUtils::ResolutionTable table = OUtils::ResolutionTable(4096); // Initial capacity of 4096 but scales easily to larger amounts

    void setupreflection(void) {
        // Add all these objects to the reflection table, it might be worth figuring out a super easy macro system so we can do this just in a header or something rather than having to write out this crap just to register a type for serialisation/deserialisation
        OUtils::reflectiontable[OUtils::fnv1a("GameObject")] = new OUtils::ReflectedImplementation<GameObject>();
        OUtils::reflectiontable[OUtils::fnv1a("SpotLight")] = new OUtils::ReflectedImplementation<SpotLight>();
        OUtils::reflectiontable[OUtils::fnv1a("PointLight")] = new OUtils::ReflectedImplementation<PointLight>();
        OUtils::reflectiontable[OUtils::fnv1a("ModelInstance")] = new OUtils::ReflectedImplementation<ModelInstance>();
    }
}
