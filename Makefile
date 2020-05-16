CXX = clang++-10 -stdlib=libc++
CXXFLAGS = -Wall -Werror -std=c++17 -g $(shell pkg-config --cflags libgit2)
LDFLAGS = $(shell pkg-config --libs libgit2)

EXE = git_fast_reword
SRCDIR = src
OBJDIR = obj

OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(wildcard $(SRCDIR)/*.cpp))

all: $(EXE) $(TESTEXE)

$(EXE): $(OBJDIR) $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(EXE) $(LDFLAGS)
	
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -MMD -o $@ $<

include $(wildcard $(OBJDIR)/*.d)

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(EXE) $(TESTEXE)

.PHONY: clean all
