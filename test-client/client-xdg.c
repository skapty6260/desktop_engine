#include <wayland-client.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include "xdg-shell-client-protocol.h"

#define WIDTH 400
#define HEIGHT 300
#define STRIDE (WIDTH * 4)

struct client_state {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct xdg_wm_base *xdg_wm_base;
    
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *toplevel;
    struct wl_buffer *buffer;
    struct wl_callback *frame_callback;  // Важно!
    
    struct wl_shm_pool *shm_pool;
    void *shm_data;
    int shm_fd;
    size_t shm_size;
    
    int running;
    int frame_count;
};

// Глобальный флаг для сигнала
static volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    struct client_state *state = data;
    if (!state) return;
    
    xdg_surface_ack_configure(xdg_surface, serial);
    
    if (state->running && state->surface && state->buffer) {
        wl_surface_attach(state->surface, state->buffer, 0, 0);
        wl_surface_damage(state->surface, 0, 0, WIDTH, HEIGHT);
        wl_surface_commit(state->surface);
    }
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                                  int32_t width, int32_t height, struct wl_array *states) {
    struct client_state *state = data;
    if (!state) return;
    
    if (width > 0 && height > 0) {
        printf("Window configured to %dx%d\n", width, height);
    }
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel) {
    struct client_state *state = data;
    if (!state) return;
    
    printf("Window close requested\n");
    state->running = 0;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xdg_toplevel_close,
};

static void registry_global(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface, uint32_t version) {
    struct client_state *state = data;
    if (!state) return;
    
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, state);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
    // Можно игнорировать
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

static int create_shm_buffer(struct client_state *state) {
    if (!state) return -1;
    
    char filename[] = "/tmp/wayland-client-shm-XXXXXX";
    state->shm_fd = mkstemp(filename);
    if (state->shm_fd < 0) {
        fprintf(stderr, "Error creating temporary file: %s\n", strerror(errno));
        return -1;
    }
    
    unlink(filename);
    
    state->shm_size = STRIDE * HEIGHT;
    if (ftruncate(state->shm_fd, state->shm_size) < 0) {
        fprintf(stderr, "Error truncating file: %s\n", strerror(errno));
        close(state->shm_fd);
        return -1;
    }
    
    state->shm_data = mmap(NULL, state->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, state->shm_fd, 0);
    if (state->shm_data == MAP_FAILED) {
        fprintf(stderr, "Error mmapping file: %s\n", strerror(errno));
        close(state->shm_fd);
        return -1;
    }
    
    struct wl_shm_pool *pool = wl_shm_create_pool(state->shm, state->shm_fd, state->shm_size);
    if (!pool) {
        fprintf(stderr, "Failed to create SHM pool\n");
        munmap(state->shm_data, state->shm_size);
        close(state->shm_fd);
        return -1;
    }
    
    state->buffer = wl_shm_pool_create_buffer(pool, 0, WIDTH, HEIGHT, STRIDE, WL_SHM_FORMAT_XRGB8888);
    
    // НЕ УНИЧТОЖАЙТЕ POOL ЗДЕСЬ!
    // wl_shm_pool_destroy(pool); // <-- УДАЛИТЕ ЭТУ СТРОКУ!
    
    // Сохраните pool в структуре для последующего уничтожения
    state->shm_pool = pool;  // Добавьте wl_shm_pool *shm_pool; в struct client_state
    
    if (!state->buffer) {
        fprintf(stderr, "Failed to create buffer\n");
        // Если buffer не создан, можно уничтожить pool
        wl_shm_pool_destroy(pool);
        munmap(state->shm_data, state->shm_size);
        close(state->shm_fd);
        return -1;
    }
    
    printf("SHM buffer created: %dx%d, stride=%d\n", WIDTH, HEIGHT, STRIDE);
    return 0;
}

static void draw_frame(struct client_state *state) {
    if (!state || !state->shm_data) {
        fprintf(stderr, "Warning: draw_frame called with invalid state\n");
        return;
    }
    
    uint32_t *pixels = (uint32_t *)state->shm_data;
    
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            uint8_t r = (x + state->frame_count) % 256;
            uint8_t g = (y + state->frame_count) % 256;
            uint8_t b = 128;
            pixels[y * WIDTH + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    state->frame_count++;
    if (state->frame_count % 100 == 0) {
        printf("Frame: %d\n", state->frame_count);
    }
}

static void frame_callback(void *data, struct wl_callback *callback, uint32_t time) {
    struct client_state *state = data;
    if (!state || !callback) return;
    
    // Уничтожаем этот callback
    wl_callback_destroy(callback);
    
    // Если state->frame_callback указывает на этот callback, очищаем
    if (state->frame_callback == callback) {
        state->frame_callback = NULL;
    }
    
    if (!state->running) {
        return;
    }
    
    // Перерисовываем
    draw_frame(state);
    if (state->surface && state->buffer) {
        wl_surface_attach(state->surface, state->buffer, 0, 0);
        wl_surface_damage(state->surface, 0, 0, WIDTH, HEIGHT);
    }
    
    // Создаем новый callback для следующего кадра
    struct wl_callback *new_callback = wl_surface_frame(state->surface);
    if (new_callback) {
        wl_callback_add_listener(new_callback, &(struct wl_callback_listener){ .done = frame_callback }, state);
        state->frame_callback = new_callback;
    }
    
    wl_surface_commit(state->surface);
}

static void create_window(struct client_state *state) {
    if (!state || !state->compositor || !state->xdg_wm_base) return;
    
    state->surface = wl_compositor_create_surface(state->compositor);
    if (!state->surface) {
        fprintf(stderr, "Failed to create surface\n");
        return;
    }
    
    state->xdg_surface = xdg_wm_base_get_xdg_surface(state->xdg_wm_base, state->surface);
    if (!state->xdg_surface) {
        fprintf(stderr, "Failed to create xdg surface\n");
        return;
    }
    
    xdg_surface_add_listener(state->xdg_surface, &xdg_surface_listener, state);
    
    state->toplevel = xdg_surface_get_toplevel(state->xdg_surface);
    if (!state->toplevel) {
        fprintf(stderr, "Failed to create toplevel\n");
        return;
    }
    
    xdg_toplevel_add_listener(state->toplevel, &xdg_toplevel_listener, state);
    xdg_toplevel_set_title(state->toplevel, "deeng wayland client testbed");
    wl_surface_commit(state->surface);
}

int main() {
    // Настройка обработчиков сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    struct client_state state = {0};
    state.running = 1;
    
    printf("Initializing Wayland client...\n");
    
    // Подключение к display
    state.display = wl_display_connect(NULL);
    if (!state.display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 1;
    }
    
    // Получение registry
    state.registry = wl_display_get_registry(state.display);
    if (!state.registry) {
        fprintf(stderr, "Failed to get registry\n");
        wl_display_disconnect(state.display);
        return 1;
    }
    
    wl_registry_add_listener(state.registry, &registry_listener, &state);
    wl_display_roundtrip(state.display);
    
    // Проверка необходимых глобальных объектов
    if (!state.compositor || !state.shm || !state.xdg_wm_base) {
        fprintf(stderr, "Missing required Wayland globals\n");
        wl_display_disconnect(state.display);
        return 1;
    }
    
    printf("Wayland globals acquired successfully\n");
    
    // Создание буфера
    if (create_shm_buffer(&state) < 0) {
        fprintf(stderr, "Failed to create SHM buffer\n");
        wl_display_disconnect(state.display);
        return 1;
    }
    
    // Создание окна
    create_window(&state);
    if (!state.surface || !state.xdg_surface || !state.toplevel) {
        fprintf(stderr, "Failed to create window\n");
        wl_display_disconnect(state.display);
        return 1;
    }
    
    // Ожидание начальной конфигурации
    wl_display_roundtrip(state.display);
    
    // Начальная отрисовка
    draw_frame(&state);
    if (state.surface && state.buffer) {
        wl_surface_attach(state.surface, state.buffer, 0, 0);
        wl_surface_damage(state.surface, 0, 0, WIDTH, HEIGHT);
    }
    
    // Запуск анимации
    state.frame_callback = wl_surface_frame(state.surface);
    if (state.frame_callback) {
        wl_callback_add_listener(state.frame_callback, &(struct wl_callback_listener){ .done = frame_callback }, &state);
    }
    wl_surface_commit(state.surface);
    
    printf("Window created, starting main loop...\n");
    
    // Главный цикл
    while (state.running && keep_running) {
        if (wl_display_dispatch(state.display) == -1) {
            fprintf(stderr, "wl_display_dispatch failed\n");
            break;
        }
    }
    
    printf("Shutting down...\n");
    
    // Остановка анимации
    state.running = 0;
    
    // Уничтожение frame callback
    if (state.frame_callback) {
        wl_callback_destroy(state.frame_callback);
        state.frame_callback = NULL;
    }
    
    // Даем Wayland обработать оставшиеся события
    wl_display_flush(state.display);
    
    // Очистка ресурсов в правильном порядке
    if (state.toplevel) xdg_toplevel_destroy(state.toplevel);
    if (state.xdg_surface) xdg_surface_destroy(state.xdg_surface);
    if (state.surface) wl_surface_destroy(state.surface);
    if (state.buffer) wl_buffer_destroy(state.buffer);
    if (state.xdg_wm_base) xdg_wm_base_destroy(state.xdg_wm_base);
    if (state.shm) wl_shm_destroy(state.shm);
    if (state.compositor) wl_compositor_destroy(state.compositor);
    if (state.registry) wl_registry_destroy(state.registry);
    
    if (state.shm_pool) {
        wl_shm_pool_destroy(state.shm_pool);
        state.shm_pool = NULL;
    }

    // Очистка SHM
    if (state.shm_data && state.shm_data != MAP_FAILED) {
        munmap(state.shm_data, state.shm_size);
    }
    if (state.shm_fd >= 0) {
        close(state.shm_fd);
    }
    
    // Отключение от display
    wl_display_disconnect(state.display);
    
    printf("Client exited successfully after %d frames\n", state.frame_count);
    return 0;
}