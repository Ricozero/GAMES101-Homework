#include <iostream>

#include "CGL/viewer.h"

#include "application.h"

int main(int argc, char **argv)
{
    Config config;
    Viewer viewer;
    // Application is deleted in ~Viewer, so it has to be a heap object
    Application *app = new Application(config, &viewer);

    viewer.set_renderer(app);
    viewer.init();
    viewer.start();

    return 0;
}
