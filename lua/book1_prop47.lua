-- SPDX-License-Identifier: GPL-3.0-or-later
-- Proposition-level behavior for Byrne / Euclid I.47.
-- C supplies exact numerical geometry through geometry.construct; this file owns
-- the proposition state machine, interaction constraints, and presentation choices.

local M ← {}

local MIN_LEG ← 0.35
local MAX_LEG ← 1.35

local finite_number ← λ(value)
    return type(value) = "number" and value = value and math.abs(value) < math.huge
end

local valid_length ← λ(value)
    return finite_number(value) and value ≥ MIN_LEG and value ≤ MAX_LEG
end

local clamp ← λ(value)
    return math.max(MIN_LEG, math.min(MAX_LEG, value))
end

local recompute ← λ(state)
    local construction, status ← geometry.construct(
        state.blue_length,
        state.yellow_length,
        state.rotation
    )
    if construction = nil then
        error("I.47 geometry rejected state; status=" .. tostring(status))
    end
    state.construction ← construction
    state.maximum_area_error ← math.max(
        state.maximum_area_error or 0.0,
        construction.area_error
    )
    return state
end

M.new ← λ()
    return recompute {
        blue_length ← 1.10,
        yellow_length ← 0.74,
        rotation ← 0.0,
        debug ← false,
        maximum_area_error ← 0.0,
    }
end

M.restore ← λ(state, blue_length, yellow_length, debug)
    if not valid_length(blue_length) or not valid_length(yellow_length) then
        return false
    end
    if type(debug) ≠ "boolean" then
        return false
    end
    state.blue_length ← blue_length
    state.yellow_length ← yellow_length
    state.debug ← debug
    recompute(state)
    return true
end

M.toggle_checks ← λ(state)
    state.debug ← not state.debug
    return state.debug
end

M.drag ← λ(state, captured, model_x, model_y)
    if captured ≠ 0 and captured ≠ 1 then
        return false
    end
    if not finite_number(model_x) or not finite_number(model_y) then
        return false
    end

    local cosine ← math.cos(state.rotation)
    local sine ← math.sin(state.rotation)
    local unit_x
    local unit_y
    if captured = 0 then
        unit_x, unit_y ← cosine, sine
    else
        unit_x, unit_y ← −sine, cosine
    end

    local projected ← model_x × unit_x + model_y × unit_y
    local updated ← clamp(projected)
    local previous ← captured = 0 and state.blue_length or state.yellow_length
    if math.abs(updated − previous) < 1e-12 then
        return false
    end

    if captured = 0 then
        state.blue_length ← updated
    else
        state.yellow_length ← updated
    end
    recompute(state)
    return true
end

M.presentation ← λ(state, width, height)
    local scale ← math.min(width × 0.90 ÷ 4.35, height × 0.48 ÷ 4.35)
    return {
        model_centre_x ← 0.675,
        model_centre_y ← 0.675,
        screen_centre_x ← width × 0.50,
        screen_centre_y ← height × 0.415,
        scale ← scale,

        checks ← {
            left ← width × 0.72,
            top ← height × 0.026,
            right ← width × 0.97,
            bottom ← height × 0.105,
        },

        y ← {
            brand ← height × 0.04,
            title ← height × 0.08,
            subtitle ← height × 0.135,
            instruction ← height × 0.695,
            legend ← height × 0.745,
            debug ← height × 0.875,
        },

        labels ← {
            brand ← "BYRNE / EUCLID",
            title ← "BOOK I.47",
            subtitle ← "THE RIGHT-ANGLED TRIANGLE",
            instruction ← "DRAG THE BLUE OR YELLOW ENDPOINT",
            equation ← "RED SQUARE = THE TWO SIDE SQUARES",
            motion ← "MOVE LEGS - AREA IDENTITY HOLDS",
            checks_on ← "CHECKS ON",
            checks_off ← "CHECKS OFF",
        },

        colours ← {
            hypotenuse_square ← "red",
            blue_square ← "blue",
            yellow_square ← "black",
            blue_leg ← "blue",
            yellow_leg ← "yellow",
            hypotenuse ← "red",
            proof ← "black",
        },
    }
end

return M
