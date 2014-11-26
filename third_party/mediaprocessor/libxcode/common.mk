# <COPYRIGHT_TAG>
export CXX=g++
export CXXFLAGS=-g -Wall -fPIC -msse4
export STATIC_LIBRARY_LINK=ar cr

export LIB_PREFIX=libxcode

LIB_NAME=$(shell basename `pwd`)
STATIC_TARGET=../../dist/$(LIB_PREFIX)$(LIB_NAME).a
DYN_TARGET=../../dist/$(LIB_PREFIX)$(LIB_NAME).so

SRCEXT=.c .cpp
SRC=$(wildcard $(addprefix *, $(SRCEXT)))
OBJDIR:=.objs
OBJS:=$(patsubst %.cpp,$(OBJDIR)/%.o,$(notdir $(SRC)))
INCLUDES_PATH+=-I. -I../

all: $(DYN_TARGET) $(STATIC_TARGET)
.PHONY:all

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES_PATH) $(EXTRA_FLAGS) -c $< -o $@

$(STATIC_TARGET): $(OBJS)
	$(STATIC_LIBRARY_LINK) $@ $(OBJS)

$(DYN_TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) -o $@ $(LIBS)

clean:
	rm -rf $(STATIC_TARGET) $(OBJDIR) $(DYN_TARGET)
