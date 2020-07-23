void get_axes_3D_positions(float tick, float m, uint coord_side, out vec3 P0, out vec3 P1, out bvec3 opposite)
{
    vec3 P00 = vec3(0), P10 = vec3(0), P01 = vec3(0);

    // uint side = coord_side % 4; // 0=x, 1=y, 2=z

    P00 = vec3(-1, -1, -1);
    for (int side = 0; side < 3; side++) {
    switch (side) {

        case 0:
        // x=-1
        P10 = vec3(-1, -1, +1);
        P01 = vec3(-1, +1, -1);
        break;

        case 1:
        // y=-1
        P10 = vec3(+1, -1, -1);
        P01 = vec3(-1, -1, +1);
        break;

        case 2:
        // z=-1
        P10 = vec3(+1, -1, -1);
        P01 = vec3(-1, +1, -1);
        break;

        default:
        break;
    }

    vec4 p00 = transform_pos(P00);
    vec4 p10 = transform_pos(P10);
    vec4 p01 = transform_pos(P01);

    // NOTE: from now on, y goes DOWN (it was inversed by transform_pos())
    // Take perspective into account before computing the sign of the determinant.
    p00.xy /= p00.w;
    p10.xy /= p10.w;
    p01.xy /= p01.w;

    mat2 m = mat2(p10.xy - p00.xy, p01.xy - p00.xy);
    opposite[side] = determinant(m) > 0;
    }
    bool opp = opposite[coord_side % 4];

    switch (coord_side) {

        case 0:
        // X horiz
        P0 = vec3(opp ? +m : -m, tick, -m);
        P1 = vec3(opp ? +m : -m, tick, +m);
        break;

        case 1:
        // Y horiz
        P0 = vec3(-m, opp ? +m : -m, tick);
        P1 = vec3(+m, opp ? +m : -m, tick);
        break;

        case 2:
        // Z horiz
        P0 = vec3(-m, tick, opp ? -m : +m);
        P1 = vec3(+m, tick, opp ? -m : +m);
        break;


        case 4:
        // X vert
        P0 = vec3(opp ? +m : -m, -m, tick);
        P1 = vec3(opp ? +m : -m, +m, tick);
        break;

        case 5:
        // Y vert
        P0 = vec3(tick, opp ? +m : -m, +m);
        P1 = vec3(tick, opp ? +m : -m, -m);
        break;

        case 6:
        // Z vert
        P0 = vec3(tick, -m, opp ? -m : +m);
        P1 = vec3(tick, +m, opp ? -m : +m);
        break;


        default:
        break;
    }
}

void get_axes_3D_positions(float tick, uint coord_side, out vec3 P0, out vec3 P1, out bvec3 opposite) {
    get_axes_3D_positions(tick, 1, coord_side, P0, P1, opposite);
}
