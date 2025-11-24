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

#define WIDTH 400
#define HEIGHT 300
#define STRIDE (WIDTH * 4)

struct client_state {
    // Wayland глобальные объекты
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    
    // Поверхность и буферы
    struct wl_surface *surface;
    struct wl_buffer *buffer;
    
    // SHM данные
    void *shm_data;
    int shm_fd;
    size_t shm_size;
    
    int running;
    int frame_count;
};

static void registry_global(void *data, struct wl_registry *registry,
                           uint32_t name, const char *interface, uint32_t version) {
    struct client_state *state = data;
    
    printf("Global available: %s (version %u)\n", interface, version);
    
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    }
}

static void registry_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
    // Глобальный объект удален
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_global_remove,
};

// Создание SHM буфера
static int create_shm_buffer(struct client_state *state) {
    // Создаем shared memory файл
    char filename[] = "/tmp/wayland-client-shm-XXXXXX";
    state->shm_fd = mkstemp(filename);
    if (state->shm_fd < 0) {
        fprintf(stderr, "Error creating temporary file\n");
        return -1;
    }
    
    unlink(filename); // Удаляем файл, но дескриптор остается
    
    // Вычисляем размер буфера
    state->shm_size = STRIDE * HEIGHT;
    if (ftruncate(state->shm_fd, state->shm_size) < 0) {
        fprintf(stderr, "Error truncating file\n");
        close(state->shm_fd);
        return -1;
    }
    
    // Маппируем shared memory
    state->shm_data = mmap(NULL, state->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, state->shm_fd, 0);
    if (state->shm_data == MAP_FAILED) {
        fprintf(stderr, "Error mmapping file\n");
        close(state->shm_fd);
        return -1;
    }
    
    // Создаем SHM пул
    struct wl_shm_pool *pool = wl_shm_create_pool(state->shm, state->shm_fd, state->shm_size);
    
    // Создаем буфер из пула
    state->buffer = wl_shm_pool_create_buffer(pool, 0, WIDTH, HEIGHT, STRIDE, WL_SHM_FORMAT_ARGB8888);
    
    // Очищаем пул
    wl_shm_pool_destroy(pool);
    
    printf("SHM buffer created: %dx%d, stride=%d\n", WIDTH, HEIGHT, STRIDE);
    return 0;
}

// Отрисовка простого градиента
static void draw_frame(struct client_state *state) {
    uint32_t *pixels = (uint32_t *)state->shm_data;
    
    // Простой анимированный градиент
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            uint8_t r = (x + state->frame_count) % 256;
            uint8_t g = (y + state->frame_count) % 256;
            uint8_t b = 128;
            
            // ARGB format
            pixels[y * WIDTH + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }
    
    state->frame_count++;
}

// Callback для frame (для анимации)
static void frame_callback(void *data, struct wl_callback *callback, uint32_t time) {
    struct client_state *state = data;
    
    // Удаляем старый callback
    wl_callback_destroy(callback);
    
    // Перерисовываем
    draw_frame(state);
    printf("\nCALLING SURFACE_ATTACH\n");
    wl_surface_attach(state->surface, state->buffer, 0, 0);
    wl_surface_damage(state->surface, 0, 0, WIDTH, HEIGHT);
    
    // Создаем новый callback для следующего кадра
    callback = wl_surface_frame(state->surface);
    wl_callback_add_listener(callback, &(struct wl_callback_listener){ .done = frame_callback }, state);
    
    wl_surface_commit(state->surface);
}

static void create_surface(struct client_state *state) {
    // Создаем поверхность
    state->surface = wl_compositor_create_surface(state->compositor);
    
    if (!state->surface) {
        fprintf(stderr, "Failed to create surface\n");
        return;
    }
    
    printf("Surface created successfully\n");
}

int main() {
    struct client_state state = {0};
    state.running = 1;
    
    printf("Initializing Wayland client (no shell protocol)...\n");
    
    // Подключаемся к Wayland display
    state.display = wl_display_connect(NULL);
    if (!state.display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return 1;
    }
    
    // Получаем registry
    state.registry = wl_display_get_registry(state.display);
    wl_registry_add_listener(state.registry, &registry_listener, &state);
    
    // Синхронизируем для получения глобальных объектов
    wl_display_roundtrip(state.display);
    
    // Проверяем необходимые глобальные объекты
    if (!state.compositor || !state.shm) {
        fprintf(stderr, "Missing required Wayland globals\n");
        return 1;
    }
    
    printf("Wayland globals acquired successfully\n");
    
    // Создаем SHM буфер
    if (create_shm_buffer(&state) < 0) {
        fprintf(stderr, "Failed to create SHM buffer\n");
        return 1;
    }
    
    // Создаем поверхность
    create_surface(&state);

    // Синхронизируем для обработки конфигурации
    wl_display_roundtrip(state.display);
    
    if (!state.surface) {
        fprintf(stderr, "Failed to create surface\n");
        return 1;
    }
    
    // Рисуем первый кадр
    draw_frame(&state);
    printf("\nCALLING SURFACE_ATTACH (for first frame)\n");
    wl_surface_attach(state.surface, state.buffer, 0, 0);
    wl_surface_damage(state.surface, 0, 0, WIDTH, HEIGHT);
    
    // Добавляем frame callback для анимации
    struct wl_callback *callback = wl_surface_frame(state.surface);
    wl_callback_add_listener(callback, &(struct wl_callback_listener){ .done = frame_callback }, &state);
    
    wl_surface_commit(state.surface);
    
    printf("Surface created and content set, starting main loop...\n");
    printf("Note: Without shell protocol, the surface may not be visible as a proper window\n");
    printf("Some compositors may show it as a fullscreen surface or not show it at all\n");
    
    // Главный цикл
    while (state.running) {
        if (wl_display_dispatch(state.display) == -1) {
            break;
        }
        
        // Простая логика выхода через 500 кадров
        if (state.frame_count > 500) {
            state.running = 0;
        }
    }
    
    printf("Shutting down...\n");
    
    // Очистка ресурсов
    if (state.buffer) wl_buffer_destroy(state.buffer);
    if (state.surface) wl_surface_destroy(state.surface);
    if (state.shm) wl_shm_destroy(state.shm);
    if (state.compositor) wl_compositor_destroy(state.compositor);
    if (state.registry) wl_registry_destroy(state.registry);
    
    // Очистка SHM
    if (state.shm_data && state.shm_data != MAP_FAILED) {
        munmap(state.shm_data, state.shm_size);
    }
    if (state.shm_fd >= 0) {
        close(state.shm_fd);
    }
    
    wl_display_disconnect(state.display);
    
    printf("Client exited successfully after %d frames\n", state.frame_count);
    return 0;
}