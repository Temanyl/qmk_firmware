TAP_DANCE_ENABLE = yes
QUANTUM_PAINTER_ENABLE = yes
QUANTUM_PAINTER_DRIVERS += st7789_spi

# Framebuffer support
SRC += display/framebuffer.c

# Display and scene rendering
SRC += display/display.c scenes/scenes.c

# Games
SRC += game_doodle.c game_tetris.c game_manager.c

# Season modules
SRC += seasons/winter/seasons_winter.c seasons/spring/seasons_spring.c seasons/summer/seasons_summer.c seasons/fall/seasons_fall.c seasons/halloween/seasons_halloween.c seasons/christmas/seasons_christmas.c seasons/easter/seasons_easter.c seasons/newyear/seasons_newyear.c

# Weather transition system
SRC += weather_transition.c

# Drawable objects
SRC += objects/seasonal/pumpkin.c objects/seasonal/ghost.c objects/seasonal/snowman.c objects/seasonal/easter_egg.c
SRC += objects/weather/smoke.c objects/weather/cloud.c objects/weather/raindrop.c
SRC += objects/celestial/sun.c objects/celestial/moon.c objects/celestial/stars.c objects/celestial/astronomical.c
SRC += objects/structures/tree.c objects/structures/cabin.c
SRC += objects/flora/flower.c objects/flora/sunflower.c objects/flora/fallen_leaf.c
SRC += objects/fauna/bird.c objects/fauna/butterfly.c objects/fauna/bee.c objects/fauna/firefly.c objects/fauna/bunny.c
SRC += objects/effects/snowflake.c objects/effects/snow_drift.c objects/effects/airplane.c
