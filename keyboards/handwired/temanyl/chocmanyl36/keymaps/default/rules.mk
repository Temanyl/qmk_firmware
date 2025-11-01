TAP_DANCE_ENABLE = yes
QUANTUM_PAINTER_ENABLE = yes
QUANTUM_PAINTER_DRIVERS += st7789_spi

# Framebuffer support
SRC += display/framebuffer.c

# Display and scene rendering
SRC += display/display.c scenes/scenes.c

# Game
SRC += game_doodle.c

# Season modules
SRC += seasons/winter/seasons_winter.c seasons/spring/seasons_spring.c seasons/summer/seasons_summer.c seasons/fall/seasons_fall.c seasons/halloween/seasons_halloween.c seasons/christmas/seasons_christmas.c

# Drawable objects
SRC += objects/seasonal/pumpkin.c objects/seasonal/ghost.c
SRC += objects/weather/smoke.c objects/weather/cloud.c objects/weather/raindrop.c
SRC += objects/celestial/sun.c objects/celestial/moon.c objects/celestial/stars.c
SRC += objects/structures/tree.c objects/structures/cabin.c
SRC += objects/flora/flower.c objects/flora/sunflower.c objects/flora/fallen_leaf.c
SRC += objects/fauna/bird.c objects/fauna/butterfly.c
SRC += objects/effects/snowflake.c objects/effects/snow_drift.c objects/effects/airplane.c
