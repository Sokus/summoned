#include "object.hpp"

Object::Object() {};

Object::Object(std::vector<std::string> tags, std::string description, std::string details, int weight, int capacity, int count)
    : tags(tags), description(description), details(details), weight(weight), capacity(capacity), count(count) { }

    
void Object::appendInv(Object& obj)
{
    if(this->inventoryHead == nullptr)
    {
        this->inventoryHead = &obj;
    }
    else
    {
        Object* p_inv = this->inventoryHead;
        while(p_inv->next != nullptr) p_inv = p_inv->next;
        p_inv->next = &obj;
    }

    obj.parent = this;
}