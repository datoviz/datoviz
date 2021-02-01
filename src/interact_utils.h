#ifndef DVZ_INTERACT_UTILS_HEADER
#define DVZ_INTERACT_UTILS_HEADER

#include "../include/datoviz/interact.h"
#include "../include/datoviz/vklite.h"



/*************************************************************************************************/
/*  Panzoom                                                                                      */
/*************************************************************************************************/

static DvzPanzoom _panzoom(DvzCanvas* canvas)
{
    DvzPanzoom p = {0};
    p.canvas = canvas;
    p.camera_pos[2] = +2;
    p.zoom[0] = 1;
    p.zoom[1] = 1;
    return p;
}

static void _panzoom_copy_prev_state(DvzPanzoom* panzoom)
{
    ASSERT(panzoom != NULL);
    glm_vec2_copy(panzoom->camera_pos, panzoom->press_pos);
    glm_vec2_copy(panzoom->zoom, panzoom->last_zoom);
}

static void _panzoom_reset(DvzPanzoom* panzoom)
{
    ASSERT(panzoom != NULL);
    glm_vec2_zero(panzoom->camera_pos);
    glm_vec2_zero(panzoom->press_pos);
    glm_vec2_one(panzoom->zoom);
    glm_vec2_one(panzoom->last_zoom);
}

static void _panzoom_pan(DvzPanzoom* panzoom, vec2 delta)
{
    ASSERT(panzoom != NULL);
    panzoom->camera_pos[0] = panzoom->press_pos[0] - delta[0] / panzoom->zoom[0];
    panzoom->camera_pos[1] = panzoom->press_pos[1] - delta[1] / panzoom->zoom[1];
}

static void _panzoom_zoom(DvzPanzoom* panzoom, vec2 delta, vec2 center)
{
    ASSERT(panzoom != NULL);
    vec2 pan, zoom_prev, zoom_new;

    // Update the zoom.
    delta[0] = CLIP(delta[0], -10, +10);
    delta[1] = CLIP(delta[1], -10, +10);
    glm_vec2_mul(panzoom->last_zoom, (vec2){exp(delta[0]), exp(delta[1])}, zoom_new);

    // Clip zoom x.
    double zx = zoom_new[0];
    if (zx <= DVZ_PANZOOM_MIN_ZOOM || zx >= DVZ_PANZOOM_MAX_ZOOM)
    {
        zoom_new[0] = CLIP(zx, DVZ_PANZOOM_MIN_ZOOM, DVZ_PANZOOM_MAX_ZOOM);
        panzoom->lim_reached[0] = true;
    }
    else
    {
        panzoom->lim_reached[0] = false;
    }
    // Clip zoom y.
    double zy = zoom_new[1];
    if (zy <= DVZ_PANZOOM_MIN_ZOOM || zy >= DVZ_PANZOOM_MAX_ZOOM)
    {
        zoom_new[1] = CLIP(zy, DVZ_PANZOOM_MIN_ZOOM, DVZ_PANZOOM_MAX_ZOOM);
        panzoom->lim_reached[1] = true;
    }
    else
    {
        panzoom->lim_reached[1] = false;
    }

    // Update zoom.
    glm_vec2_copy(panzoom->zoom, zoom_prev);
    if (!panzoom->lim_reached[0])
        panzoom->zoom[0] = zoom_new[0];
    if (!panzoom->lim_reached[1])
        panzoom->zoom[1] = zoom_new[1];

    // Update pan.
    pan[0] = -center[0] * (1.0f / zoom_prev[0] - 1.0f / zoom_new[0]) * zoom_new[0];
    pan[1] = -center[1] * (1.0f / zoom_prev[1] - 1.0f / zoom_new[1]) * zoom_new[1];

    if (!panzoom->lim_reached[0])
        panzoom->camera_pos[0] -= pan[0] / panzoom->zoom[0];
    if (!panzoom->lim_reached[1])
        panzoom->camera_pos[1] -= pan[1] / panzoom->zoom[1];
}

static void _panzoom_update_mvp(DvzPanzoom* panzoom, DvzMVP* mvp)
{
    ASSERT(panzoom != NULL);
    // View matrix (depends on the pan).
    {
        vec3 center;
        glm_vec3_copy(panzoom->camera_pos, center);
        center[2] = 0.0f; // only the z coord changes between panel and center.
        vec3 lookup = {0, 1, 0};
        glm_lookat(panzoom->camera_pos, center, lookup, mvp->view);
    }
    // Proj matrix (depends on the zoom).
    {
        float zx = panzoom->zoom[0];
        float zy = panzoom->zoom[1];
        glm_ortho(-1.0f / zx, +1.0f / zx, -1.0f / zy, 1.0f / zy, -10.0f, 10.0f, mvp->proj);
    }
}

static void _panzoom_callback(
    DvzInteract* interact, DvzViewport viewport, DvzMouse* mouse, DvzKeyboard* keyboard)
{
    ASSERT(interact != NULL);
    ASSERT(
        interact->type == DVZ_INTERACT_PANZOOM ||
        interact->type == DVZ_INTERACT_PANZOOM_FIXED_ASPECT);
    DvzPanzoom* panzoom = &interact->u.p;
    bool is_active = false;

    // Update the last camera/zoom variables.
    if (mouse->prev_state == DVZ_MOUSE_STATE_INACTIVE && mouse->cur_state == DVZ_MOUSE_STATE_DRAG)
        _panzoom_copy_prev_state(panzoom);

    float wheel_factor = DVZ_PANZOOM_MOUSE_WHEEL_FACTOR;
    vec2 delta = {0};

#if OS_MACOS
    // HACK: touchpad wheel too sensitive on macOS
    wheel_factor *= -.1;
#endif

    bool cur_active = _pos_in_viewport(viewport, mouse->cur_pos);
    bool press_active = _pos_in_viewport(viewport, mouse->press_pos);

    // Pan.
    if (press_active && mouse->cur_state == DVZ_MOUSE_STATE_DRAG &&
        mouse->button == DVZ_MOUSE_BUTTON_LEFT)
    {
        // TODO
        // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        // if (dvz_panel_from_mouse(scene, mouse->press_pos) != panel)
        //     return;
        // panel->status = DVZ_PANEL_STATUS_ACTIVE;
        glm_vec2_sub(interact->mouse_local.cur_pos, interact->mouse_local.press_pos, delta);
        _panzoom_pan(panzoom, delta);
        is_active = true;
    } // end pan

    // Zoom.
    if ((mouse->cur_state == DVZ_MOUSE_STATE_DRAG && mouse->button == DVZ_MOUSE_BUTTON_RIGHT) ||
        mouse->cur_state == DVZ_MOUSE_STATE_WHEEL)
    {
        vec2 center = {0};

        // Right drag.
        if (press_active && mouse->cur_state == DVZ_MOUSE_STATE_DRAG &&
            mouse->button == DVZ_MOUSE_BUTTON_RIGHT)
        {
            // TODO
            // Restrict the panzoom updates to cases when the mouse press position was in the
            // panel.
            // if (dvz_panel_from_mouse(scene, mouse->press_pos) != panel)
            //     return;
            // panel->status = DVZ_PANEL_STATUS_ACTIVE;

            // Get the center position: mouse press position.
            glm_vec2_copy(interact->mouse_local.press_pos, center);
            glm_vec2_sub(interact->mouse_local.cur_pos, interact->mouse_local.press_pos, delta);

            delta[0] *= 1.5;
            delta[1] *= 1.5;
            is_active = true;
        }
        // Mouse wheel.
        else if (cur_active && mouse->cur_state == DVZ_MOUSE_STATE_WHEEL)
        {
            // TODO
            // Restrict the panzoom updates to cases when the mouse press position was in the
            // panel.
            // if (dvz_panel_from_mouse(scene, mouse->cur_pos) != panel)
            //     return;
            // panel->status = DVZ_PANEL_STATUS_ACTIVE;

            glm_vec2_copy(interact->mouse_local.cur_pos, center);
            glm_vec2_copy(panzoom->zoom, panzoom->last_zoom);

            delta[0] = delta[1] = mouse->wheel_delta[1] * wheel_factor;
            is_active = true;
        }

        // Fixed aspect ratio.
        if (is_active)
        {
            if (panzoom->fixed_aspect)
                delta[0] = delta[1] = .5 * (delta[0] + delta[1]);
            _panzoom_zoom(panzoom, delta, center);
        }
    } // end zoom

    // Reset on double-click.
    if (cur_active && mouse->cur_state == DVZ_MOUSE_STATE_DOUBLE_CLICK)
    {
        // TODO
        // // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        // if (dvz_panel_from_mouse(scene, mouse->cur_pos) != panel)
        //     return;
        // panel->status = DVZ_PANEL_STATUS_RESET;

        _panzoom_reset(panzoom);
        is_active = true;
    }

    if (mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE)
    {
        // Reset the last camera/zoom variables.
        glm_vec2_zero(panzoom->press_pos);
        glm_vec2_one(panzoom->last_zoom);

        // TODO
        //     panel->status = DVZ_PANEL_STATUS_NONE;
    }

    if (is_active)
        _panzoom_update_mvp(panzoom, &interact->mvp);
    interact->is_active = is_active;
}



/*************************************************************************************************/
/*  Camera                                                                                       */
/*************************************************************************************************/

static void _camera_reset(DvzCamera* camera)
{
    ASSERT(camera != NULL);
    glm_vec3_copy(DVZ_CAMERA_EYE, camera->eye);
    glm_vec3_copy(camera->eye, camera->target);
    glm_vec3_copy((vec3){0, 0, -1}, camera->forward);
    glm_vec3_copy(DVZ_CAMERA_UP, camera->up);
}

static DvzCamera _camera(DvzCanvas* canvas, DvzInteractType type)
{
    DvzCamera c = {0};
    c.canvas = canvas;
    _camera_reset(&c);
    return c;
}

static void _camera_update_mvp(DvzCamera* camera, DvzMVP* mvp)
{
    ASSERT(camera != NULL);
    vec3 center = {0};
    glm_vec3_add(camera->eye, camera->forward, center);
    dvz_mvp_camera(camera->canvas->viewport, camera->eye, center, (vec2){0.1, 100}, mvp);
}

static void _camera_callback(
    DvzInteract* interact, DvzViewport viewport, DvzMouse* mouse, DvzKeyboard* keyboard)
{
    ASSERT(interact != NULL);
    DvzCamera* camera = &interact->u.c;
    DvzMouseLocal* mouse_local = &interact->mouse_local;
    bool is_fly = interact->type == DVZ_INTERACT_FLY;
    bool is_active = false;

    const float dt = (float)interact->canvas->clock.interval;
    const float alpha = 5;
    const float beta = 10;
    const float dl = alpha * dt;
    const float max_pitch = .99;

    // Variables for the look-around camera with the mouse.
    vec3 yaw_axis, pitch_axis;
    glm_vec3_zero(yaw_axis);
    glm_vec3_zero(pitch_axis);
    yaw_axis[1] = 1;
    mat4 rot;
    float ymin = -10;
    vec3 advance = {0};

    switch (interact->type)
    {

    case DVZ_INTERACT_FPS:
    case DVZ_INTERACT_FLY:
        if (mouse->cur_state == DVZ_MOUSE_STATE_DRAG)
        {
            // Change the camera orientation with the mouse.
            camera->speed = 0.5f;
            float incrx = -(mouse_local->cur_pos[0] - mouse_local->last_pos[0]) * camera->speed;
            float incry = -(mouse_local->cur_pos[1] - mouse_local->last_pos[1]) * camera->speed;

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
        if (mouse->cur_state == DVZ_MOUSE_STATE_WHEEL)
        {
            camera->target[1] += 1 * dl * mouse->wheel_delta[1];
        }

        // Arrow keys navigation.
        if (keyboard->cur_state == DVZ_KEYBOARD_STATE_ACTIVE)
        {
            if (keyboard->key_code == DVZ_KEY_UP)
            {
                advance[0] = +camera->forward[0];
                advance[1] = +camera->forward[1];
                advance[2] = +camera->forward[2];
            }
            else if (keyboard->key_code == DVZ_KEY_DOWN)
            {
                advance[0] = -camera->forward[0];
                advance[1] = -camera->forward[1];
                advance[2] = -camera->forward[2];
            }
            else if (keyboard->key_code == DVZ_KEY_RIGHT)
            {
                advance[0] = -camera->forward[2];
                advance[2] = +camera->forward[0];
            }
            else if (keyboard->key_code == DVZ_KEY_LEFT)
            {
                advance[0] = +camera->forward[2];
                advance[2] = -camera->forward[0];
            }
        }

        break;

    default:
        break;
    }

    // Reset the camera on double click.
    if (mouse->cur_state == DVZ_MOUSE_STATE_DOUBLE_CLICK)
    {
        _camera_reset(camera);
        return;
        // panel->status = DVZ_PANEL_STATUS_RESET;
    }

    // Add the advance vector to the target position.
    if (!is_fly)
        advance[1] = 0;
    if (glm_vec3_norm(advance) > 1e-3)
    {
        glm_vec3_normalize(advance);
        glm_vec3_scale(advance, dl, advance);
        glm_vec3_add(camera->target, advance, camera->target);
        is_active = true;
    }

    // Smooth move.
    vec3 u = {0};
    glm_vec3_sub(camera->target, camera->eye, u);
    if (glm_vec3_norm(u) > 1e-3)
    {
        glm_vec3_scale(u, beta * dt, u);
        glm_vec3_add(camera->eye, u, camera->eye);

        // Prevent going below y=0 plane.
        if (!is_fly)
            ymin = 0;
        camera->eye[1] = CLIP(camera->eye[1], ymin, 10);
        camera->target[1] = CLIP(camera->target[1], ymin, 10);
    }

    _camera_update_mvp(camera, &interact->mvp);
    interact->is_active = is_active;
}



/*************************************************************************************************/
/*  Arcball                                                                                      */
/*************************************************************************************************/

// adapted from https://github.com/Twinklebear/arcball-cpp/blob/master/arcball_panel.cpp

static void _arcball_reset(DvzArcball* arcball)
{
    ASSERT(arcball != NULL);

    vec3 eye, center, up, dir, x_axis, y_axis, z_axis;
    glm_vec3_copy(DVZ_CAMERA_EYE, arcball->camera.eye);
    glm_vec3_copy(arcball->camera.eye, eye);
    glm_vec3_copy((vec3){0, 0, 0}, center);
    glm_vec3_copy(DVZ_CAMERA_UP, up);

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

    glm_mat4_identity(arcball->center_translation);
    glm_translate_make(arcball->translation, (vec3){0, 0, -glm_vec3_norm(dir)});

    mat3 m;
    glm_vec3_copy((vec3){-x_axis[0], -x_axis[1], -x_axis[2]}, m[0]);
    glm_vec3_copy((vec3){+y_axis[0], +y_axis[1], +y_axis[2]}, m[1]);
    glm_vec3_copy((vec3){-z_axis[0], -z_axis[1], -z_axis[2]}, m[2]);

    glm_mat3_transpose(m);
    glm_mat3_quat(m, arcball->rotation);
    glm_quat_normalize(arcball->rotation);
}

static DvzArcball _arcball(DvzCanvas* canvas)
{
    DvzArcball arcball = {0};
    arcball.canvas = canvas;
    _arcball_reset(&arcball);
    return arcball;
}

static void _screen_to_arcball(vec2 p, versor q)
{
    float dist = glm_vec2_dot(p, p);
    // If we're on/in the sphere return the point on it
    if (dist <= 1.f)
    {
        glm_vec4_copy((vec4){p[0], p[1], sqrt(1 - dist), 0}, q);
    }
    else
    {
        // otherwise we project the point onto the sphere
        glm_vec2_normalize(p);
        glm_vec4_copy((vec4){p[0], p[1], 0, 0}, q);
    }
}

static void _arcball_rotate(DvzArcball* arcball, vec2 cur_pos, vec2 last_pos)
{
    ASSERT(arcball != NULL);

    // // NOTE: need to invert the mouse normalized coordinates if the standard 3D view matrix
    // is
    // // also applied.
    // if (arcball->which_matrix == DVZ_MVP_MODEL)
    // {
    //     cur_pos[0] *= -1;
    //     cur_pos[1] *= -1;
    //     last_pos[0] *= -1;
    //     last_pos[1] *= -1;
    // }

    versor mouse_cur_ball = {0}, mouse_prev_ball = {0};
    _screen_to_arcball(cur_pos, mouse_cur_ball);
    _screen_to_arcball(last_pos, mouse_prev_ball);

    glm_quat_mul(mouse_prev_ball, arcball->rotation, arcball->rotation);
    glm_quat_mul(mouse_cur_ball, arcball->rotation, arcball->rotation);
}

static void _arcball_update_mvp(DvzArcball* arcball, DvzMVP* mvp)
{
    ASSERT(arcball != NULL);
    glm_mat4_copy(arcball->mat, mvp->model);
    dvz_mvp_camera(
        arcball->canvas->viewport, arcball->camera.eye, (vec3){0, 0, 0}, (vec2){0.1, 100}, mvp);
}

static void _arcball_callback(
    DvzInteract* interact, DvzViewport viewport, DvzMouse* mouse, DvzKeyboard* keyboard)
{
    ASSERT(interact != NULL);
    ASSERT(interact->type == DVZ_INTERACT_ARCBALL);
    DvzArcball* arcball = &interact->u.a;
    bool is_active = true;

    bool cur_active = _pos_in_viewport(viewport, mouse->cur_pos);
    bool press_active = _pos_in_viewport(viewport, mouse->press_pos);

    // Rotate.
    if (press_active && mouse->cur_state == DVZ_MOUSE_STATE_DRAG &&
        mouse->button == DVZ_MOUSE_BUTTON_LEFT)
    {
        // // TODO
        // // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        // if (dvz_panel_from_mouse(scene, mouse->press_pos) != panel)
        //     return;
        // panel->status = DVZ_PANEL_STATUS_ACTIVE;

        _arcball_rotate(arcball, interact->mouse_local.cur_pos, interact->mouse_local.last_pos);
        is_active = true;
    }

    // Zoom.
    if (cur_active && mouse->cur_state == DVZ_MOUSE_STATE_WHEEL)
    {
        // // TODO
        // // Restrict the panzoom updates to cases when the mouse press position was in the panel.
        // if (dvz_panel_from_mouse(scene, mouse->cur_pos) != panel)
        //     return;
        // panel->status = DVZ_PANEL_STATUS_ACTIVE;
        glm_vec3_scale(arcball->camera.eye, exp(.1 * mouse->wheel_delta[1]), arcball->camera.eye);
        is_active = true;
    }

    // Reset with double-click.
    if (mouse->cur_state == DVZ_MOUSE_STATE_DOUBLE_CLICK)
    {
        _arcball_reset(arcball);
        // panel->status = DVZ_PANEL_STATUS_RESET;
        is_active = true;
    }

    // Compute the View matrix.
    glm_quat_mat4(arcball->rotation, arcball->mat);
    // glm_mat4_mul(arcball->mat, arcball->center_translation, arcball->mat);
    // glm_mat4_mul(arcball->translation, arcball->mat, arcball->mat);

    // // Pan.
    // if (press_active && mouse->cur_state == DVZ_MOUSE_STATE_DRAG &&
    //     mouse->button == DVZ_MOUSE_BUTTON_RIGHT)
    // {
    //     // // TODO
    //     // if (dvz_panel_from_mouse(scene, mouse->press_pos) != panel)
    //     //     return;
    //     // panel->status = DVZ_PANEL_STATUS_ACTIVE;

    //     vec2 delta;
    //     glm_vec2_sub(interact->mouse_local.cur_pos, interact->mouse_local.last_pos, delta);
    //     float k = 5;
    //     vec3 motion = {k * delta[0], k * delta[1], 0};
    //     glm_vec3_add(arcball->camera.eye, motion, arcball->camera.eye);

    //     is_active = true;
    // }

    if (is_active)
        _arcball_update_mvp(arcball, &interact->mvp);
    interact->is_active = is_active;

    // if (mouse->cur_state == DVZ_MOUSE_STATE_INACTIVE)
    // {
    //     panel->status = DVZ_PANEL_STATUS_NONE;
    // }
}



#endif
