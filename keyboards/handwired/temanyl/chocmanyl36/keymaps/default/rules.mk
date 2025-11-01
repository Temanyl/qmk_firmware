TAP_DANCE_ENABLE = yes
QUANTUM_PAINTER_ENABLE = yes
QUANTUM_PAINTER_DRIVERS += st7789_spi

# Framebuffer support
SRC += framebuffer.c

# Display and scene rendering
SRC += display.c scenes.c

# Season modules
SRC += seasons_winter.c seasons_spring.c seasons_summer.c seasons_fall.c seasons_halloween.c seasons_christmas.c

# Drawable objects
SRC += objects/seasonal/pumpkin.c objects/seasonal/ghost.c
