NAME = webserv

CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -g -std=c++98 -fsanitize=address

SRCDIR = src
INCDIR = include
OBJDIR = obj

SRCS = $(SRCDIR)/main.cpp \
       $(SRCDIR)/config/Config.cpp \
       $(SRCDIR)/config/uri.cpp \
       $(SRCDIR)/config/ConfigRouter.cpp \
       $(SRCDIR)/config/ConfigParser.cpp \
       $(SRCDIR)/config/Location.cpp \
       $(SRCDIR)/config/ConfigValidator.cpp \
       $(SRCDIR)/config/ConfigNode.cpp \
       $(SRCDIR)/config/ServerConfig.cpp\
       $(SRCDIR)/server/Server.cpp \
       $(SRCDIR)/server/Client.cpp \
       $(SRCDIR)/server/EventLoop.cpp \
       $(SRCDIR)/http/HttpRequest.cpp \
       $(SRCDIR)/http/HttpResponse.cpp \
       $(SRCDIR)/http/HttpParser.cpp \
       $(SRCDIR)/http/MimeTypes.cpp \
       $(SRCDIR)/http/RequestHandler.cpp \
       $(SRCDIR)/cgi/CgiHandler.cpp \
       $(SRCDIR)/utils/Utils.cpp \
       $(SRCDIR)/utils/Logger.cpp \
       $(SRCDIR)/utils/socket_utils.cpp

# src/%.cpp files to obj/%.o files
OBJS = $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -MMD -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

test: all
	@bash tests/run_tests.sh

.PHONY: all clean fclean re test
