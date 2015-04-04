CC = g++ -Wall -ansi -DLINUX -DNO_JPEG -mmmx `pkg-config --cflags --libs gtk+-2.0`
RELEASE = -O2 -ffast-math
DEBUG = -g

FW_PATH  = ../Framework3
APP_NAME = DeferredLighting

FW_BASE = $(FW_PATH)/Linux/LinuxBase.cpp $(FW_PATH)/CPU.cpp $(FW_PATH)/Platform.cpp
FW_APP = $(FW_PATH)/BaseApp.cpp $(FW_PATH)/OpenGL/OpenGLApp.cpp $(FW_PATH)/Config.cpp $(FW_PATH)/Util/Tokenizer.cpp $(FW_PATH)/Util/String.cpp
FW_RENDERER = $(FW_PATH)/Renderer.cpp $(FW_PATH)/OpenGL/OpenGLRenderer.cpp $(FW_PATH)/OpenGL/project.cpp $(FW_PATH)/OpenGL/OpenGLExtensions.cpp $(FW_PATH)/Imaging/Image.cpp
FW_MATH = $(FW_PATH)/Math/Vector.cpp $(FW_PATH)/Math/Scissor.cpp
FW_GUI = $(FW_PATH)/GUI/Widget.cpp $(FW_PATH)/GUI/Button.cpp $(FW_PATH)/GUI/Dialog.cpp $(FW_PATH)/GUI/CheckBox.cpp $(FW_PATH)/GUI/Slider.cpp $(FW_PATH)/GUI/Label.cpp $(FW_PATH)/GUI/DropDownList.cpp
FW_UTIL =  $(FW_PATH)/Util/Model.cpp $(FW_PATH)/Util/BSP.cpp
FW = $(FW_BASE) $(FW_APP) $(FW_RENDERER) $(FW_MATH) $(FW_GUI) $(FW_UTIL)
APP = App.cpp App_Util.cpp

rel: $(APP) $(FW)
	$(CC) $(RELEASE) $(APP) $(FW) -o $(APP_NAME) -L/usr/X11R6/lib -lGL -lXxf86vm -L/usr/lib -lpng
dbg: $(APP) $(FW)
	$(CC) $(DEBUG) $(APP) $(FW) -o $(APP_NAME) -L/usr/X11R6/lib -lGL -lXxf86vm -L/usr/lib -lpng

clean:
	@rm $(APP_NAME)
