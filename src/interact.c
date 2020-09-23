#include "../include/visky/interact.h"



/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/

VkyPanzoom* vky_panzoom_init()
{
    // NOTE: the caller must free the panzoom.

    // Allocate the VkyPanzoom instance on the heap.
    VkyPanzoom* panzoom = (VkyPanzoom*)calloc(1, sizeof(VkyPanzoom));

    // Initialize the panzoom properties.
    panzoom->camera_pos[0] = VKY_PANZOOM_LOOKAT_POS_X;
    panzoom->camera_pos[1] = VKY_PANZOOM_LOOKAT_POS_Y;
    panzoom->camera_pos[2] = VKY_PANZOOM_LOOKAT_POS_Z;
    glm_vec2_copy((vec2){1.0f, 1.0f}, panzoom->zoom);

    glm_mat4_identity(panzoom->model);

    return panzoom;
}

void vky_panzoom_update(VkyPanel* panel, VkyPanzoom* panzoom, VkyViewportType viewport_type)
{
    VkyScene* scene = panel->scene;
    VkyMouse* mouse = scene->canvas->event_controller->mouse;
    VkyViewport viewport = vky_get_viewport(panel, viewport_type);
    float aspect_ratio = panel->aspect_ratio;
    float wheel_factor = VKY_PANZOOM_MOUSE_WHEEL_FACTOR;

#if OS_MACOS
    // HACK: touchpad wheel too sensitive on macOS
    wheel_factor *= -.1;
#endif

    // Pan.
    if (mouse->cur_state == VKY_MOUSE_STATE_DRAG && mouse->button == VKY_MOUSE_BUTTON_LEFT)
    {

        // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        if (vky_panel_from_mouse(scene, mouse->press_pos) != panel)
            return;
        panel->is_active = true;

        vec2 delta = {0};
        vky_mouse_move_delta(mouse, viewport, delta);

        if (aspect_ratio == 1)
        {
            delta[0] *= scene->canvas->size.framebuffer_width /
                        (float)scene->canvas->size.framebuffer_height;
        }

        panzoom->camera_pos[0] -= delta[0] / panzoom->zoom[0];
        panzoom->camera_pos[1] -= delta[1] / panzoom->zoom[1];
    } // end pan

    // Zoom.
    if ((mouse->cur_state == VKY_MOUSE_STATE_DRAG && mouse->button == VKY_MOUSE_BUTTON_RIGHT) ||
        mouse->cur_state == VKY_MOUSE_STATE_WHEEL)
    {
        vec2 pan, delta, zoom_old, zoom_new, center;

        // Right drag.
        if (mouse->cur_state == VKY_MOUSE_STATE_DRAG && mouse->button == VKY_MOUSE_BUTTON_RIGHT)
        {

            // Restrict the panzoom updates to cases when the mouse press position was in the
            // panel.
            if (vky_panel_from_mouse(scene, mouse->press_pos) != panel)
                return;
            panel->is_active = true;

            // Get the center position: mouse press position.
            vky_mouse_press_pos(mouse, viewport, center);

            // Get the mouse move delta.
            vky_mouse_move_delta(mouse, viewport, delta);
            // Transform back the delta in pixels.
            delta[0] *= viewport.w / 2;
            delta[1] *= viewport.h / 2;
            delta[0] *= .0025;
            delta[1] *= .0025;
        }
        // Mouse wheel.
        else
        {

            // Restrict the panzoom updates to cases when the mouse press position was in the
            // panel.
            if (vky_panel_from_mouse(scene, mouse->cur_pos) != panel)
                return;
            panel->is_active = true;

            vky_mouse_cur_pos(mouse, viewport, center);
            glm_vec2_copy(mouse->wheel_delta, delta);
            delta[0] *= wheel_factor;
            delta[1] *= wheel_factor;
        }

        // Fixed aspect ratio.
        if (aspect_ratio == 1)
        {
            delta[0] = delta[1] = copysignf(1.0, delta[0] + delta[1]) *
                                  sqrt(delta[0] * delta[0] + delta[1] * delta[1]);

            center[0] *= scene->canvas->size.framebuffer_width /
                         (float)scene->canvas->size.framebuffer_height;
        }

        // Update the zoom.
        glm_vec2_copy(panzoom->zoom, zoom_old);
        glm_vec2_copy(panzoom->zoom, zoom_new);
        glm_vec2_mul(zoom_new, (vec2){delta[0] + 1, delta[1] + 1}, zoom_new);

        // Clip zoom x.
        double zx = zoom_new[0];
        if (zx <= VKY_PANZOOM_MIN_ZOOM || zx >= VKY_PANZOOM_MAX_ZOOM)
        {
            zoom_new[0] = CLIP(zx, VKY_PANZOOM_MIN_ZOOM, VKY_PANZOOM_MAX_ZOOM);
            panzoom->lim_reached[0] = true;
        }
        else
        {
        }
        // Clip zoom y.
        double zy = zoom_new[1];
        if (zy <= VKY_PANZOOM_MIN_ZOOM || zy >= VKY_PANZOOM_MAX_ZOOM)
        {
            zoom_new[1] = CLIP(zy, VKY_PANZOOM_MIN_ZOOM, VKY_PANZOOM_MAX_ZOOM);
            panzoom->lim_reached[1] = true;
        }
        else
        {
        }

        // Update zoom.
        if (!panzoom->lim_reached[0])
            panzoom->zoom[0] = zoom_new[0];
        if (!panzoom->lim_reached[1])
            panzoom->zoom[1] = zoom_new[1];

        // Update pan.
        pan[0] = -center[0] * (1.0f / zoom_old[0] - 1.0f / zoom_new[0]) * zoom_new[0];
        pan[1] = -center[1] * (1.0f / zoom_old[1] - 1.0f / zoom_new[1]) * zoom_new[1];

        if (!panzoom->lim_reached[0])
            panzoom->camera_pos[0] -= pan[0] / panzoom->zoom[0];
        if (!panzoom->lim_reached[1])
            panzoom->camera_pos[1] -= pan[1] / panzoom->zoom[1];
    } // end zoom

    // Reset on double-click.
    if (mouse->cur_state == VKY_MOUSE_STATE_DOUBLE_CLICK)
    {

        // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        if (vky_panel_from_mouse(scene, mouse->cur_pos) != panel)
            return;
        panel->is_active = false;

        panzoom->camera_pos[0] = 0;
        panzoom->camera_pos[1] = 0;
        panzoom->zoom[0] = 1;
        panzoom->zoom[1] = 1;
    }

    if (mouse->cur_state == VKY_MOUSE_STATE_STATIC)
    {
        panel->is_active = false;
    }
}

void vky_panzoom_mvp(VkyPanel* panel, VkyPanzoom* panzoom, VkyViewportType viewport_type)
{
    VkyScene* scene = panel->scene;
    float aspect_ratio = panel->aspect_ratio;

    // Declare the matrices.
    mat4 view, proj;
    // glm_mat4_identity(panzoom->model);
    glm_mat4_identity(view);
    glm_mat4_identity(proj);

    // View matrix (depends on the pan).
    vec3 center;
    glm_vec3_copy(panzoom->camera_pos, center);
    center[2] = 0.0f; // only the z coord changes between panel and center.
    vec3 lookup = {
        VKY_PANZOOM_LOOKAT_UP_X,
        VKY_PANZOOM_LOOKAT_UP_Y,
        VKY_PANZOOM_LOOKAT_UP_Z,
    };
    glm_lookat(panzoom->camera_pos, center, lookup, view);

    // Proj matrix (depends on the zoom).
    float zx = panzoom->zoom[0];
    float zy = panzoom->zoom[1];
    // TODO: other aspect ratios
    if (aspect_ratio == 1)
    {
        zx *=
            scene->canvas->size.framebuffer_height / (float)scene->canvas->size.framebuffer_width;
    }
    glm_ortho(-1.0f / zx, +1.0f / zx, -1.0f / zy, 1.0f / zy, -10.0f, 10.0f, proj);

    // Update the panels with the MVP matrices.
    VkyMVP mvp = vky_create_mvp();
    vky_mvp_set_model(panzoom->model, &mvp);
    vky_mvp_set_view(view, &mvp);
    vky_mvp_set_proj(proj, &mvp);
    mvp.aspect_ratio = aspect_ratio;
    vky_mvp_upload(panel, viewport_type, &mvp);
    vky_mvp_finalize(scene);
}



/*************************************************************************************************/
/*  Arcball                                                                                      */
/*************************************************************************************************/

// adapted from https://github.com/Twinklebear/arcball-cpp/blob/master/arcball_panel.cpp

static void _reset_arcball(VkyArcball* arcball)
{

    vec3 eye, center, up, dir, x_axis, y_axis, z_axis;
    glm_vec3_copy(arcball->eye_init, eye);
    glm_vec3_copy((vec3){0, 0, 0}, center);
    glm_vec3_copy((vec3){0, +1, 0}, up);

    glm_vec3_sub(center, eye, dir);
    glm_vec3_copy(dir, z_axis);
    glm_vec3_normalize(z_axis);
    glm_vec3_normalize(up);

    glm_vec3_cross(z_axis, up, x_axis);
    glm_vec3_normalize(x_axis);

    glm_vec3_cross(x_axis, z_axis, y_axis);
    glm_vec3_normalize(y_axis);

    glm_vec3_cross(z_axis, y_axis, x_axis);
    glm_vec3_normalize(x_axis);

    glm_translate_make(arcball->center_translation, center);
    glm_mat4_inv(arcball->center_translation, arcball->center_translation);

    glm_translate_make(arcball->translation, (vec3){0, 0, -glm_vec3_norm(dir)});

    mat3 m;
    glm_vec3_copy((vec3){-x_axis[0], -x_axis[1], -x_axis[2]}, m[0]);
    glm_vec3_copy((vec3){+y_axis[0], +y_axis[1], +y_axis[2]}, m[1]);
    glm_vec3_copy((vec3){+z_axis[0], +z_axis[1], +z_axis[2]}, m[2]);

    glm_mat3_transpose(m);
    glm_mat3_quat(m, arcball->rotation);
    glm_quat_normalize(arcball->rotation);

    glm_mat4_identity(arcball->mat_user);
}

VkyArcball* vky_arcball_init(VkyMVPMatrix which_matrix)
{
    // Allocate the VkyArcball instance on the heap.
    VkyArcball* arcball = (VkyArcball*)calloc(1, sizeof(VkyArcball));
    if (which_matrix == VKY_MVP_VIEW)
    {
        arcball->eye_init[2] = 2;
    }
    arcball->which_matrix = which_matrix;

    _reset_arcball(arcball);

    arcball->mvp = vky_create_mvp();

    if (which_matrix == VKY_MVP_MODEL)
    {
        // If the arcball modifies the model matrix, the view matrix should be set to the standard
        // 3D view matrix.
        vky_mvp_set_view_3D(
            (vec3)VKY_DEFAULT_CAMERA_POS, (vec3)VKY_DEFAULT_CAMERA_CENTER, &arcball->mvp);
    }

    return arcball;
}

void vky_screen_to_arcball(vec2 p, versor q)
{
    float dist = glm_vec2_dot(p, p);
    // If we're on/in the sphere return the point on it
    if (dist <= 1.f)
    {
        glm_vec4_copy((vec4){p[0], -p[1], sqrt(1 - dist), 0}, q);
    }
    else
    {
        // otherwise we project the point onto the sphere
        glm_vec2_normalize(p);
        glm_vec4_copy((vec4){p[0], -p[1], 0, 0}, q);
    }
}

void vky_arcball_update(VkyPanel* panel, VkyArcball* arcball, VkyViewportType viewport_type)
{
    VkyScene* scene = panel->scene;
    VkyMouse* mouse = scene->canvas->event_controller->mouse;
    VkyViewport viewport = vky_get_viewport(panel, viewport_type);

    // Rotate.
    if (mouse->cur_state == VKY_MOUSE_STATE_DRAG && mouse->button == VKY_MOUSE_BUTTON_LEFT)
    {
        vec2 cur_pos, last_pos;

        // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        if (vky_panel_from_mouse(scene, mouse->press_pos) != panel)
            return;
        panel->is_active = true;

        vky_mouse_cur_pos(mouse, viewport, cur_pos);
        vky_mouse_last_pos(mouse, viewport, last_pos);

        // NOTE: need to invert the mouse normalized coordinates if the standard 3D view matrix is
        // also applied.
        if (arcball->which_matrix == VKY_MVP_MODEL)
        {
            cur_pos[0] *= -1;
            cur_pos[1] *= -1;
            last_pos[0] *= -1;
            last_pos[1] *= -1;
        }

        versor mouse_cur_ball = {0}, mouse_prev_ball = {0};
        vky_screen_to_arcball(cur_pos, mouse_cur_ball);
        vky_screen_to_arcball(last_pos, mouse_prev_ball);

        glm_quat_mul(mouse_prev_ball, arcball->rotation, arcball->rotation);
        glm_quat_mul(mouse_cur_ball, arcball->rotation, arcball->rotation);
    }

    // Zoom.
    if (mouse->cur_state == VKY_MOUSE_STATE_WHEEL)
    {

        // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        if (vky_panel_from_mouse(scene, mouse->cur_pos) != panel)
            return;
        panel->is_active = true;

        vec3 motion = {0, 0, -2 * mouse->wheel_delta[1]};
        mat4 tr;
        glm_translate_make(tr, motion);
        glm_mat4_mul(tr, arcball->translation, arcball->translation);
        // Zoom bound.
        // float zoom_max = 1;
        // if (arcball->translation[3][2] > -zoom_max)
        //     arcball->translation[3][2] = -zoom_max;
    }

    // Reset with double-click.
    if (mouse->cur_state == VKY_MOUSE_STATE_DOUBLE_CLICK)
    {
        _reset_arcball(arcball);
        panel->is_active = false;
    }

    // Compute the View matrix.
    mat4 arcball_mat;
    glm_quat_mat4(arcball->rotation, arcball_mat);
    glm_mat4_mul(arcball_mat, arcball->center_translation, arcball_mat);
    glm_mat4_mul(arcball->translation, arcball_mat, arcball_mat);

    // Pan.
    if (mouse->cur_state == VKY_MOUSE_STATE_DRAG && mouse->button == VKY_MOUSE_BUTTON_RIGHT)
    {
        if (vky_panel_from_mouse(scene, mouse->press_pos) != panel)
            return;
        panel->is_active = true;

        // float zoom_amount = 1;//abs(arcball->translation[3][2]);
        vec2 delta;
        vky_mouse_move_delta(mouse, viewport, delta);

        // NOTE: need to invert the mouse normalized coordinates if the standard 3D view matrix is
        // also applied.
        if (arcball->which_matrix == VKY_MVP_MODEL)
        {
            delta[0] *= -1;
            delta[1] *= -1;
        }

        vec4 motion = {
            VKY_ARCBALL_PAN_FACTOR * delta[0], VKY_ARCBALL_PAN_FACTOR * -delta[1], 0, 0};
        // Find the panning amount in the world space
        mat4 inv_panel;
        glm_mat4_inv(arcball_mat, inv_panel);
        glm_mat4_mulv(inv_panel, motion, motion);
        mat4 tr;
        glm_translate_make(tr, motion);
        glm_mat4_mul(tr, arcball->center_translation, arcball->center_translation);

        // Update the view matrix.
        glm_quat_mat4(arcball->rotation, arcball_mat);
        glm_mat4_mul(arcball_mat, arcball->center_translation, arcball_mat);
        glm_mat4_mul(arcball->translation, arcball_mat, arcball_mat);
    }

    // Make a copy of the transformation matrix, if other controllers or the user want to modify
    // the model/view matrix.
    glm_mat4_copy(arcball_mat, arcball->mat_arcball);

    // Take the user matrix into account.
    glm_mat4_mul(arcball->mat_arcball, arcball->mat_user, arcball->mat_arcball);

    // Update the panels with the MVP matrices.
    if (arcball->which_matrix == VKY_MVP_MODEL)
    {
        vky_mvp_set_model(arcball->mat_arcball, &arcball->mvp);
    }
    else if (arcball->which_matrix == VKY_MVP_VIEW)
    {
        vky_mvp_set_view(arcball->mat_arcball, &arcball->mvp);
    }
    else
    {
        log_error("the matrix passed to the arcball controller should be either the model or the "
                  "view matrix");
    }
    vky_mvp_set_proj_3D(panel, viewport_type, &arcball->mvp);
    vky_mvp_upload(panel, viewport_type, &arcball->mvp);
    vky_mvp_finalize(scene);

    if (mouse->cur_state == VKY_MOUSE_STATE_STATIC)
    {
        panel->is_active = false;
    }
}



/*************************************************************************************************/
/*  FPS camera                                                                                   */
/*************************************************************************************************/

static void _reset_camera(VkyCamera* camera)
{
    glm_vec3_copy((vec3)VKY_DEFAULT_CAMERA_POS, camera->eye);
    glm_vec3_copy(camera->eye, camera->target);
    glm_vec3_copy((vec3){0, 0, -1}, camera->forward);
    glm_vec3_copy((vec3)VKY_DEFAULT_CAMERA_UP, camera->up);

    camera->mvp = vky_create_mvp();
}

VkyCamera* vky_camera_init()
{
    VkyCamera* camera = (VkyCamera*)calloc(1, sizeof(VkyCamera));
    _reset_camera(camera);
    return camera;
}

void vky_camera_update(VkyPanel* panel, VkyCamera* camera, VkyViewportType viewport_type)
{
    VkyScene* scene = panel->scene;
    VkyControllerType controller_type = panel->controller_type;
    VkyKeyboard* keyboard = scene->canvas->event_controller->keyboard;
    VkyMouse* mouse = scene->canvas->event_controller->mouse;

    // double t = scene->canvas->local_time;
    const float increment = .35;
    const float max_pitch = .99;

    // Variables for the look-around camera with the mouse.
    vec3 yaw_axis, pitch_axis;
    glm_vec3_zero(yaw_axis);
    glm_vec3_zero(pitch_axis);
    yaw_axis[1] = 1;
    mat4 rot;
    float ymin = VKY_CAMERA_YMIN;

    switch (controller_type)
    {

    case VKY_CONTROLLER_FPS:
    case VKY_CONTROLLER_FLY:
        if (mouse->cur_state == VKY_MOUSE_STATE_DRAG)
        {
            // Change the camera orientation with the mouse.
            vec2 mouse_delta;
            vky_mouse_move_delta(mouse, panel->viewport, mouse_delta);
            camera->speed = VKY_CAMERA_SENSITIVITY;

            float incrx = mouse_delta[0] * camera->speed;
            float incry = mouse_delta[1] * camera->speed;

            glm_rotate_make(rot, incrx, yaw_axis);
            glm_mat4_mulv3(rot, camera->forward, 1, camera->forward);

            glm_vec3_crossn(camera->up, camera->forward, pitch_axis);
            if ((camera->forward[1] > max_pitch && incry > 0) ||
                (camera->forward[1] < -max_pitch && incry < 0))
                incry = 0;
            glm_rotate_make(rot, incry, pitch_axis);
            glm_mat4_mulv3(rot, camera->forward, 1, camera->forward);
        }

        // Change the camera elevation with the mouse wheel.
        if (mouse->cur_state == VKY_MOUSE_STATE_WHEEL)
        {
            camera->target[1] -= mouse->wheel_delta[1];
        }

        // Arrow keys navigation.
        vec2 forward_plane;
        forward_plane[0] = camera->forward[0];
        forward_plane[1] = camera->forward[2];
        glm_vec2_normalize(forward_plane);

        if (keyboard->key == VKY_KEY_UP)
        {
            if (controller_type == VKY_CONTROLLER_FPS)
            {
                camera->target[0] += increment * forward_plane[0];
                camera->target[2] += increment * forward_plane[1];
            }
            else if (controller_type == VKY_CONTROLLER_FLY)
            {
                camera->target[0] += increment * camera->forward[0];
                camera->target[1] += increment * camera->forward[1];
                camera->target[2] += increment * camera->forward[2];
            }
        }
        else if (keyboard->key == VKY_KEY_DOWN)
        {
            if (controller_type == VKY_CONTROLLER_FPS)
            {
                camera->target[0] -= increment * forward_plane[0];
                camera->target[2] -= increment * forward_plane[1];
            }
            else if (controller_type == VKY_CONTROLLER_FLY)
            {
                camera->target[0] -= increment * camera->forward[0];
                camera->target[1] -= increment * camera->forward[1];
                camera->target[2] -= increment * camera->forward[2];
            }
        }
        else if (keyboard->key == VKY_KEY_RIGHT)
        {
            camera->target[0] -= -increment * camera->forward[2];
            camera->target[2] -= increment * camera->forward[0];
        }
        else if (keyboard->key == VKY_KEY_LEFT)
        {
            camera->target[0] += -increment * camera->forward[2];
            camera->target[2] += increment * camera->forward[0];
        }

        // Smooth move.
        const double alpha = 10 * scene->canvas->dt;
        camera->eye[0] += alpha * (camera->target[0] - camera->eye[0]);
        camera->eye[1] += alpha * (camera->target[1] - camera->eye[1]);
        camera->eye[2] += alpha * (camera->target[2] - camera->eye[2]);
        break;

    case VKY_CONTROLLER_AUTOROTATE:
        camera->speed = VKY_CAMERA_SPEED * scene->canvas->dt;
        glm_rotate_make(rot, camera->speed, yaw_axis);
        glm_mat4_mulv3(rot, camera->eye, 1, camera->eye);
        glm_mat4_mulv3(rot, camera->forward, 1, camera->forward);
        break;

    default:
        break;
    }

    // Prevent going below y=0 plane.
    if (controller_type == VKY_CONTROLLER_FPS)
        ymin = 0;

    camera->eye[1] = CLIP(camera->eye[1], ymin, VKY_CAMERA_YMAX);
    camera->target[1] = CLIP(camera->target[1], ymin, VKY_CAMERA_YMAX);

    // Reset the camera on double click.
    if (mouse->cur_state == VKY_MOUSE_STATE_DOUBLE_CLICK)
    {
        _reset_camera(camera);
        panel->is_active = false;
    }

    glm_vec3_normalize(camera->forward);
    glm_look(camera->eye, camera->forward, camera->up, camera->mvp.view);
    vky_mvp_set_proj_3D(panel, viewport_type, &camera->mvp);
    vky_mvp_upload(panel, viewport_type, &camera->mvp);
    vky_mvp_finalize(scene);
}
