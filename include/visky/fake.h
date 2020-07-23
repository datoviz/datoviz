/*************************************************************************************************/
/*  Fake sphere visual                                                                           */
/*************************************************************************************************/

typedef struct VkyFakeSphereParams VkyFakeSphereParams;
struct VkyFakeSphereParams
{
    vec4 light_pos;
};

typedef struct VkyFakeSphereVertex VkyFakeSphereVertex;
struct VkyFakeSphereVertex
{
    vec3 pos;
    VkyColorBytes color;
    float radius;
};

VKY_EXPORT VkyVisual* vky_visual_fake_sphere(VkyScene* scene, VkyFakeSphereParams params);
