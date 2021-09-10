execute_process(COMMAND ${python} "${COMPONENT_DIR}/src/fontscan.py")

set(srcs "src/screen.c" "src/draw.c" "src/bw_screen.c" "src/v_screen.c" "src/vbit_screen.c" "src/bitmap.c" "src/font.c" "src/interscreen.c")

if(CONFIG_DGX_ENABLE_SPI)
	list(APPEND srcs "src/spi_screen.c")
endif()

if(CONFIG_DGX_ENABLE_I2C)
	list(APPEND srcs "src/i2c_screen.c")
endif()

if(CONFIG_DGX_ENABLE_P8)
	list(APPEND srcs "src/p8_screen.c")
endif()

if(CONFIG_DGX_ENABLE_SPI_ST7735)
	list(APPEND srcs "src/drivers/spi_st7735.c")
endif()

if(CONFIG_DGX_ENABLE_SPI_ST7789)
	list(APPEND srcs "src/drivers/spi_st7789.c")
endif()

if(CONFIG_DGX_ENABLE_SPI_SSD_1306)
	list(APPEND srcs "src/drivers/spi_ssd1306.c")
endif()

if(CONFIG_DGX_ENABLE_SPI_ILI_9341)
	list(APPEND srcs "src/drivers/spi_ili9341.c")
endif()

if(CONFIG_DGX_ENABLE_I2C_SSD_1306)
	list(APPEND srcs "src/drivers/i2c_ssd1306.c")
endif()

if(CONFIG_DGX_ENABLE_P8_ILI_9341)
	list(APPEND srcs "src/drivers/p8_ili9341.c")
endif()

file(STRINGS "${COMPONENT_DIR}/src/fonts/fonts.txt" fonts)

if(fonts)
	list(APPEND srcs ${fonts})
endif()

idf_component_register(SRCS "${srcs}" INCLUDE_DIRS "include")
