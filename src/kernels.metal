#include <metal_stdlib>
using namespace metal;

struct Match {
    float cam_u;
    float cam_v;
    float proj_u;
    float proj_v;
};

// 0..3: fx, fy, cx, cy
// 4..8: k1, k2, p1, p2, k3
// 9..17: r00..r22
// 18..20: ox, oy, oz

struct PointOutput {
    packed_float3 point;
    int valid;
};

float3 pixelToRay(float u, float v, constant float* cam) {
    float fx = cam[0], fy = cam[1], cx = cam[2], cy = cam[3];
    float k1 = cam[4], k2 = cam[5], p1 = cam[6], p2 = cam[7], k3 = cam[8];
    
    float x = (u - cx) / fx;
    float y = (v - cy) / fy;
    float x0 = x;
    float y0 = y;
    for(int j = 0; j < 5; j++) {
        float r2 = x*x + y*y;
        float r4 = r2*r2;
        float r6 = r4*r2;
        
        float icdist = 1.0 / (1.0 + k1*r2 + k2*r4 + k3*r6);
        float deltaX = 2.0*p1*x*y + p2*(r2 + 2.0*x*x);
        float deltaY = p1*(r2 + 2.0*y*y) + 2.0*p2*x*y;
        
        x = (x0 - deltaX) * icdist;
        y = (y0 - deltaY) * icdist;
    }
    
    // ray_world = R * ray_camera
    float r00 = cam[9], r01 = cam[10], r02 = cam[11];
    float r10 = cam[12], r11 = cam[13], r12 = cam[14];
    float r20 = cam[15], r21 = cam[16], r22 = cam[17];

    float rx = r00 * x + r01 * y + r02 * 1.0;
    float ry = r10 * x + r11 * y + r12 * 1.0;
    float rz = r20 * x + r21 * y + r22 * 1.0;
    
    return normalize(float3(rx, ry, rz));
}

kernel void intersect_rays(device const Match* matches [[buffer(0)]],
                           device PointOutput* outputs [[buffer(1)]],
                           constant float* params [[buffer(2)]],
                           uint id [[thread_position_in_grid]]) {
    Match m = matches[id];
    
    constant float* cam = &params[0];
    constant float* proj = &params[21];
    float zMin = params[42];
    float zMax = params[43];

    float3 o1 = float3(cam[18], cam[19], cam[20]);
    float3 d1 = pixelToRay(m.cam_u, m.cam_v, cam);
    
    float3 o2 = float3(proj[18], proj[19], proj[20]);
    float3 d2 = pixelToRay(m.proj_u, m.proj_v, proj);
    
    float3 c = o2 - o1;
    float d1_dot_d2 = dot(d1, d2);
    float d1_dot_d1 = dot(d1, d1);
    float d2_dot_d2 = dot(d2, d2);
    
    float det = d1_dot_d1 * d2_dot_d2 - d1_dot_d2 * d1_dot_d2;
    float t1 = 0;
    float t2 = 0;
    
    float3 final_pt;
    if (abs(det) < 1e-5) {
        final_pt = (o1 + o2) / 2.0;
    } else {
        float c_dot_d1 = dot(c, d1);
        float c_dot_d2 = dot(c, d2);
        
        t1 = (c_dot_d1 * d2_dot_d2 - c_dot_d2 * d1_dot_d2) / det;
        t2 = (c_dot_d1 * d1_dot_d2 - c_dot_d2 * d1_dot_d1) / det;
        
        float3 P1 = o1 + t1 * d1;
        float3 P2 = o2 + t2 * d2;
        final_pt = (P1 + P2) / 2.0;
    }
    
    if (final_pt.z >= zMin && final_pt.z <= zMax) {
        outputs[id].point = final_pt;
        outputs[id].valid = 1;
    } else {
        outputs[id].valid = 0;
    }
}
