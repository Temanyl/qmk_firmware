/*
Copyright 2022 Joe Scotto

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

/**
 * Unified Object Interface
 *
 * All drawable/animated objects in the scene system follow this standardized interface.
 * This allows scene managers to be object-agnostic and use uniform patterns.
 *
 * ============================================================================
 * STANDARD METHODS (all objects should implement where applicable)
 * ============================================================================
 *
 * 1. Initialization:
 *    void <type>_init(<type>_t* obj, ...params)
 *
 *    - Initializes a single object instance with starting parameters
 *    - Called once per object when creating/resetting
 *    - Example: bird_init(&bird, x, base_y, velocity_x, bob_phase)
 *
 * 2. Drawing:
 *    void <type>_draw(const <type>_t* obj, ...extra_params)
 *
 *    - Draws a single object instance to the framebuffer
 *    - May take additional parameters (e.g., cloud_type, season)
 *    - Should not modify object state (uses const)
 *    - Example: bird_draw(&bird)
 *
 * 3. Update (animated objects only):
 *    void <type>_update(<type>_t* obj)
 *
 *    - Updates a single object instance's animation state
 *    - Called each frame for animated objects
 *    - Static objects may omit this method
 *    - Example: bird_update(&bird)
 *
 * 4. Bounds:
 *    void <type>_get_bounds(const <type>_t* obj, int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2)
 *
 *    - Returns bounding box for a single object instance
 *    - Used for region-based rendering optimization
 *    - Used for collision/occlusion detection
 *    - Example: bird_get_bounds(&bird, &x1, &y1, &x2, &y2)
 *
 * 5. Point Containment:
 *    bool <type>_contains_point(const <type>_t* obj, int16_t px, int16_t py)
 *
 *    - Tests if a point is inside the object's visual bounds
 *    - Used for hit testing and layering
 *    - Example: if (bird_contains_point(&bird, 120, 50)) { ... }
 *
 * ============================================================================
 * ARRAY MANAGEMENT PATTERN
 * ============================================================================
 *
 * Objects operate on SINGLE INSTANCES only. Scene managers own the arrays.
 *
 * In scene manager (e.g., seasons_spring.c):
 *
 *     #define NUM_SPRING_BIRDS 6
 *     bird_t birds[NUM_SPRING_BIRDS];  // Scene manager owns array
 *
 *     // Configuration data (owned by scene manager)
 *     static const struct {
 *         uint16_t base_y;
 *         float velocity_x;
 *         float bob_phase;
 *     } bird_config[NUM_SPRING_BIRDS] = {
 *         {50, 0.25f, 0.0f},
 *         {40, 0.35f, 1.0f},
 *         // ...
 *     };
 *
 *     // Initialization
 *     void init_spring_animations(void) {
 *         for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
 *             bird_init(&birds[i],
 *                       i * 25.0f + 15.0f,
 *                       bird_config[i].base_y,
 *                       bird_config[i].velocity_x,
 *                       bird_config[i].bob_phase);
 *         }
 *     }
 *
 *     // Animation loop
 *     void animate_spring(void) {
 *         // Update all birds
 *         for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
 *             bird_update(&birds[i]);
 *         }
 *
 *         // Draw all birds
 *         for (uint8_t i = 0; i < NUM_SPRING_BIRDS; i++) {
 *             bird_draw(&birds[i]);
 *         }
 *     }
 *
 * ============================================================================
 * OBJECT CATEGORIES & TYPICAL IMPLEMENTATIONS
 * ============================================================================
 *
 * Static Objects (tree, cabin, sun, moon):
 *   - Implement: init, draw
 *   - Optional: get_bounds (if used in layering)
 *   - Omit: update, contains_point
 *
 * Simple Particles (raindrop, snowflake, smoke):
 *   - Implement: init, draw, update, get_bounds
 *   - Optional: contains_point (rarely needed)
 *   - Characteristics: minimal state, simple movement
 *
 * Complex Animated Objects (bird, bee, butterfly, bunny, firefly):
 *   - Implement: ALL methods
 *   - Characteristics: rich animation state, complex behaviors
 *   - Examples: wing flapping, orbital motion, wandering
 *
 * ============================================================================
 * BENEFITS OF THIS DESIGN
 * ============================================================================
 *
 * 1. **Separation of Concerns:**
 *    - Objects handle rendering logic
 *    - Scene managers handle lifecycle & organization
 *
 * 2. **Object-Agnostic Scenes:**
 *    - Same loop pattern for all object types
 *    - Easy to add new object types
 *
 * 3. **Testability:**
 *    - Objects can be tested individually
 *    - No hidden dependencies on global state
 *
 * 4. **Maintainability:**
 *    - Consistent naming across all objects
 *    - Clear ownership of arrays
 *    - Easy to understand and modify
 *
 * ============================================================================
 * MIGRATION NOTES
 * ============================================================================
 *
 * This interface was established to replace older patterns where objects
 * managed their own internal arrays (e.g., birds_init(), bees_draw_all()).
 *
 * Old pattern (deprecated):
 *     extern bird_state_t birds[];  // In bird.h
 *     void birds_init(void);        // Managed internal array
 *     void birds_draw_all(void);    // Drew all instances
 *
 * New pattern (current):
 *     // In scene manager:
 *     bird_t birds[NUM_BIRDS];      // Scene manager owns array
 *     bird_init(&birds[i], ...);    // Initialize single instance
 *     bird_draw(&birds[i]);         // Draw single instance
 */
