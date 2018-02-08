#ifndef SAMPLER_H
#define SAMPLER_H

#include "glm/vec3.hpp"
#include "glm/glm.hpp"
#include <math.h>
#include <vector>
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/point_types.h>

using namespace glm;

class Sampler
{
  public:
    std::vector<vec3> tris;
    std::vector<vec3> normals;
    int num_tris;
    double tarea;
    double *weights;

    bool flip_flag;

    Sampler(const char *path, bool flip)
    {
        bool res = loadOBJ(path, tris, normals);
        std::cout << "read: " << (res ? "sucessful" : "fail") << std::endl;

        num_tris = int(tris.size() / 3);
        weights = new double[num_tris];
        tarea = 0;
        for (int i = 0; i < num_tris; i++)
        {
            double carea = getTriArea(tris[i * 3], tris[i * 3 + 1], tris[i * 3 + 2]);
            weights[i] = carea;
            tarea += carea;
        }
        for (int i = 0; i < num_tris; i++)
        {
            weights[i] /= tarea;
        }

        flip_flag = flip;
        std::cout << "modela surface area: " << tarea << std::endl;
    }

    double getTriArea(const vec3 &a, const vec3 &b, const vec3 &c)
    {
        double e0 = distance(a, b);
        double e1 = distance(b, c);
        double e2 = distance(c, a);
        double s = (e0 + e1 + e2) / 2;
        double area = std::sqrt(s * (s - e0) * (s - e1) * (s - e2));
        return area;
    }

    pcl::PointCloud<pcl::PointNormal> getPointCloud(int sample_density)
    {
        int num_samples = int(sample_density * tarea);
        pcl::PointCloud<pcl::PointNormal> cloud;
        cloud.width = num_samples;
        cloud.height = 1;
        cloud.is_dense = false;
        cloud.points.resize(num_samples);

        for (int i = 0; i < num_samples; i++)
        {
            //for every sample, randomly choose a tri
            int tri_index = 0;
            double x = ((double)std::rand() / (RAND_MAX));
            for (int j = 0; j < num_samples; j++)
            {
                if (x <= weights[j])
                {
                    tri_index = j;
                    break;
                }
                x -= weights[j];
            }

            //std::cout << tri_index << std::endl;
            //for a random tri, randomly select a point
            //interpolate the normal
            vec3 pt_n;
            vec3 pt = getRandomPtOnTri(tris[tri_index * 3], tris[tri_index * 3 + 1], tris[tri_index * 3 + 2], normals[tri_index * 3], normals[tri_index * 3 + 1], normals[tri_index * 3 + 2], pt_n);
            //write to cloud
            cloud.points[i].x = pt.x;
            cloud.points[i].y = pt.y;
            cloud.points[i].z = pt.z;
            cloud.points[i].normal_x = pt_n.x;
            cloud.points[i].normal_y = pt_n.y;
            cloud.points[i].normal_z = pt_n.z;
        }
        return cloud;
    }

    vec3 getRandomPtOnTri(const vec3 &a, const vec3 &b, const vec3 &c,
                          const vec3 &a_n, const vec3 &b_n, const vec3 &c_n,
                          vec3 &normal)
    {
        float r0 = ((double)std::rand() / (RAND_MAX));
        float r1 = ((double)std::rand() / (RAND_MAX));
        vec3 e0 = b - a;
        vec3 e1 = c - a;
        vec3 pt = a + r0 * e0 + r1 * e1;
        //is pt in abc?
        //http://blackpawn.com/texts/pointinpoly/
        //if not r0=1-r0, r1=1-r1
        //calculate the barycentric coord
        //interpolate normal
        vec3 e2 = pt - a;
        float dot00 = dot(e0, e0);
        float dot01 = dot(e0, e1);
        float dot02 = dot(e0, e2);
        float dot11 = dot(e1, e1);
        float dot12 = dot(e1, e2);
        float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
        float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
        float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
        if ((u >= -0.00001) && (v >= -0.00001) && (u + v <= 1))
        {
            float tarea = float(getTriArea(a, b, c));
            float area0 = float(getTriArea(b, c, pt));
            float area1 = float(getTriArea(c, a, pt));
            float area2 = float(getTriArea(a, c, pt));
            float w0 = area0 / tarea;
            float w1 = area1 / tarea;
            float w2 = area2 / tarea;
            // float w0 =
            // float w1 = ((c.y - a.y) * (pt.x - c.x) + (a.x - c.x) * (pt.y - c.y)) / ((b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y));
            // float w2 = 1.0 - w0 - w1;
            // std::cout<<((b.y - c.y) * (a.x - c.x) + (c.x - b.x) * (a.y - c.y))<<std::endl;
            //std::cout << "tarea: " << tarea << ", area0: " << area0 << std::endl;
            //std::cout << "w0: " << w0 << ", w1: " << w1 << ", w2: " << w2 << std::endl;
            vec3 pt_n = w0 * a_n + w1 * b_n + w2 * c_n;
            if (flip_flag)
                pt_n = -pt_n;
            normal = normalize(pt_n);
            return pt;
        }
        else
        {
            r0 = 1.0 - r0;
            r1 = 1.0 - r1;
            pt = a + r0 * e0 + r1 * e1;
            e2 = pt - a;
            dot00 = dot(e0, e0);
            dot01 = dot(e0, e1);
            dot02 = dot(e0, e2);
            dot11 = dot(e1, e1);
            dot12 = dot(e1, e2);
            invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
            u = (dot11 * dot02 - dot01 * dot12) * invDenom;
            v = (dot00 * dot12 - dot01 * dot02) * invDenom;
            if ((u >= -0.00001) && (v >= -0.00001) && (u + v <= 1))
            {
                float tarea = float(getTriArea(a, b, c));
                float area0 = float(getTriArea(b, c, pt));
                float area1 = float(getTriArea(c, a, pt));
                float area2 = float(getTriArea(a, c, pt));
                float w0 = area0 / tarea;
                float w1 = area1 / tarea;
                float w2 = area2 / tarea;
                vec3 pt_n = w0 * a_n + w1 * b_n + w2 * c_n;
                if (flip_flag)
                    pt_n = -pt_n;
                normal = normalize(pt_n);
                return pt;
            }
            else
            {
                std::cout << "u: " << u << ", v: " << v << std::endl;
                std::cout << "r0: " << r0 << ", r1: " << r1 << std::endl;
                std::cout << "pt: " << to_string(pt) << std::endl;
                std::cout << "a: " << to_string(a) << ", b: " << to_string(b) << ", c: " << to_string(c) << std::endl;
                std::cout << "wrong implementation" << std::endl;
                while (true)
                {
                    ;
                }
            }
        }
    }
};

#endif