//
//  texture.h
//  OGLHCubeView
//
//  Created by Gabriele on 05/07/16.
//  Copyright © 2016 Gabriele. All rights reserved.
//
#pragma once
#include <vector>
#include <string>
#include <OpenGL4.h>
#include <resource.h>
#include <smart_pointers.h>

class texture : public smart_pointers<texture>, public resource
{
  
    unsigned int m_texture_id{ 0 };
    unsigned long m_width    { 0 };
    unsigned long m_height   { 0 };
    
public:
    
    struct attributes
    {
        int  m_type_image{ GL_RGB     };
        int  m_min_filter{ GL_NEAREST };
        int  m_mag_filter{ GL_NEAREST };
        bool m_clamp_to_border { false };
        bool m_build_mipmap    { false };
        bool m_alpha_channel   { false };
        
        attributes(int  type_image,
                   int  min_filter      = GL_NEAREST,
                   int  mag_filter      = GL_NEAREST,
                   bool clamp_to_border = false,
                   bool build_mipmap    = false,
                   bool alpha_channel   = false)
        {
            m_type_image      = type_image;
            m_min_filter      = min_filter;
            m_mag_filter      = mag_filter;
            m_clamp_to_border = clamp_to_border;
            m_build_mipmap    = build_mipmap;
            m_alpha_channel   = alpha_channel;
        }
        
        static attributes rgba(int  min_filter      = GL_NEAREST,
                               int  mag_filter      = GL_NEAREST,
                               bool clamp_to_border = false,
                               bool build_mipmap    = false,
                               bool alpha_channel   = true)
        {
            return
            {
                GL_RGBA,
                min_filter,
                mag_filter,
                clamp_to_border,
                build_mipmap,
                alpha_channel
            };
        }
        
        static attributes rgba_linear(bool clamp_to_border = false,
                                      bool alpha_channel   = true)
        {
            return
            {
                GL_RGBA,
                GL_LINEAR,
                GL_LINEAR,
                clamp_to_border,
                false,
                alpha_channel
            };
        }
        
        static attributes rgba_linear_mipmap(bool clamp_to_border = false,
                                             bool alpha_channel   = true)
        {
            return
            {
                GL_RGBA,
                GL_LINEAR_MIPMAP_LINEAR,
                GL_LINEAR,
                clamp_to_border,
                true,
                alpha_channel
            };
        }
    };
    
    texture();
    
    texture(const std::string& path);
    
    texture(const attributes& attr,
            const std::string& path);
    
    texture(const attributes& attr,
            const unsigned char* buffer,
            unsigned long width,
            unsigned long height);
    
    texture(const attributes& attr,
            const std::vector< unsigned char >& buffer,
            unsigned long width,
            unsigned long height);
    
    virtual ~texture();
    
    bool load(const std::string& path);
    
    bool load(const attributes& attr,
              const std::string& path);
    
    
    bool build(const attributes& attr,
               const unsigned char* buffer,
               unsigned long width,
               unsigned long height);
    
    bool build(const attributes& attr,
               const std::vector< unsigned char >& buffer,
               unsigned long width,
               unsigned long height);
    
    unsigned int get_id() const;
    unsigned long get_width() const;
    unsigned long get_height() const;

    void destoy();
    
};