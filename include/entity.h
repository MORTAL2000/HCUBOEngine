//
//  entity.h
//  OGLHCubeView
//
//  Created by Gabriele on 02/07/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#include <unordered_map>
#include <material.h>
#include <glm/mat4x4.hpp>
#include <light.h>
#include <renderable.h>
#include <smart_pointers.h>
#include <component.h>
#include <type_traits>
//owanner
class system_manager;
//entity
class entity : public smart_pointers< entity >
{
    
    friend class system_manager;
    
public:
    
    //default
    entity();
    
    //component list
    entity( std::initializer_list<component_ptr> components );
    
    //auto add component list
    template < typename ...Args >
    entity( component_ptr component, Args&& ... args ) : entity(args...)
    {
        add_component(component);
    }
    
    template < class T >
    T& add_component()
    {
        static_assert(std::is_base_of<component,T>::value,"Must to be a component");
        return add_component(new T());
    }
    component_ptr add_component(component_ptr component_t);
    
    template < class T >
    std::shared_ptr< T > get_component()
    {
        static_assert(std::is_base_of<component,T>::value,"Must to be a component");
        return std::static_pointer_cast< T >( m_components[T::type()] );
    }
    component_ptr get_component(component_id id);
    
    template < class T >
    std::shared_ptr< T > remove_component()
    {
        static_assert(std::is_base_of<component,T>::value,"Must to be a component");
        return std::static_pointer_cast<T>(remove_component(T::type()));
    }
    component_ptr remove_component(component_id id);
    
    
    template < class T >
    bool has_component() const
    {
        static_assert(std::is_base_of<component,T>::value,"Must to be a component");
        return has_component(T::type());
    }
    bool has_component(component_id id);
    
    bool has_component(component_id id) const;
    
    entity* get_parent() const;
    
    system_manager* get_system() const;
    
    bool has_child(entity::ptr entity) const;
    
    void add_child(entity::ptr entity);
    
    void remove_child(const entity::ptr entity);
    
    bool on_update(double deltatime);
    
    void send_message_to_component(const message& message);
    
    void send_message_to_component_upwards(const message& message);
    
    void send_message_to_component_downwards(const message& message);
    
    void send_message_to_component(component_id id, const message& message);
    
    void send_message_to_component_upwards(component_id id,const message& message);
    
    void send_message_to_component_downwards(component_id id,const message& message);

protected:
    //system events
    virtual void on_attach( system_manager& );
    virtual void on_detach();
    //parent
    system_manager* m_system{ nullptr };
    entity* m_parent{ nullptr };
    //list components
    std::unordered_map< component_id,component_ptr > m_components;
    std::unordered_map< entity*,entity::ptr > m_entities;
    
};
