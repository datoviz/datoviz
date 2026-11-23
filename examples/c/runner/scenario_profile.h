/* Shared CPU presentation policy for native and WASM scenario runners. */
#pragma once

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct
{
    int kind;
    double amplitude_deg;
    double period_seconds;
} ScenarioPresentation;

typedef struct
{
    const char* id;
    ScenarioPresentation presentation;
} ScenarioPresentationEntry;

#include "scenario_profiles.h"

/**
 * Resolve the generated presentation default for a scenario.
 * @param id Scenario identifier.
 * @return Shared presentation configuration.
 */
static inline ScenarioPresentation scenario_presentation_default(const char* id)
{
    for (size_t i = 0; scenario_presentation_entries[i].id != NULL; i++)
        if (id != NULL && strcmp(id, scenario_presentation_entries[i].id) == 0)
            return scenario_presentation_entries[i].presentation;
    return (ScenarioPresentation){0, 0, 14.4};
}



/**
 * Validate a host-provided presentation configuration.
 * @param profile Presentation configuration.
 * @return Whether the configuration can be applied.
 */
static inline bool scenario_presentation_valid(ScenarioPresentation profile)
{
    return profile.kind >= 0 && profile.kind <= 2 && isfinite(profile.amplitude_deg) &&
           profile.amplitude_deg >= 0 && profile.amplitude_deg <= 180 &&
           isfinite(profile.period_seconds) && profile.period_seconds > 0;
}



/**
 * Evaluate presentation rotation independently of scientific animation time.
 * @param profile Presentation configuration.
 * @param elapsed Presentation time in output seconds.
 * @return Angle in radians.
 */
static inline double scenario_presentation_angle(ScenarioPresentation profile, double elapsed)
{
    if (!scenario_presentation_valid(profile) || profile.kind != 2 || !isfinite(elapsed))
        return 0;
    const double tau = 6.283185307179586;
    return profile.amplitude_deg * (tau / 360.0) *
           sin(tau * fmod(elapsed / profile.period_seconds, 1.0));
}
