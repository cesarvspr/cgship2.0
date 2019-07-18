#ifndef _AGL_H_
#define _AGL_H_

#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <GL/glew.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include "log.h"
#include "types.h"

/* O proposito dessa library abstrata é criar layer para simplificar o uso
   de todas as funcionalidades grafica do projeto.
*/

namespace agl {

// um ponto 2d(horizontal ponto)
struct Point2 {
  float x, z;
};

// um vetor 3d
struct Point3 {
  float x, y, z;

  Point3(float x = 0.0f, float y = 0.0f, float z = 0.0f);
  // Point3();

  inline void gl_translate() { glTranslatef(x, y, z); }

  float modulo() const;

  Point3 normalize() const;

  Point3 operator-() const;

  Point3 &operator+=(const Point3 &other);

  Point3 operator+(const Point3 &other) const;
  Point3 operator-(const Point3 &other) const;
  Point3 operator/(float f) const;
  
  Point3 operator%(const Point3 &a) const;
};

using Vec3 = Point3;

struct Normal3 : public Vec3 {
  Normal3(float x = 0.0f, float y = 0.0f, float z = 0.0f);
  Normal3(const Vec3 &vec3);

  void render() const;
};

struct Vertex {
  Point3 point;
  Normal3 normal;

  Vertex(const Point3 &p = Point3());

  void render(bool send_normal = false) const;
};

struct Edge {
public:
  Vertex *v[2]; // ponteiros para 2 bordas extremas
};

struct Face {
public:
  Vertex *verts[3]; // ponteiros para 3 vértices

  // public
  Face(Vertex *a, Vertex *b, Vertex *c);

  // cada face tem uma normal 
  Normal3 normal; 

  // computa a normal da face
  inline void computeNormal() {
    normal = -((verts[1]->point - verts[0]->point) %
               (verts[2]->point - verts[0]->point))
                  .normalize();
  }
};

// Cria objeto intrelaçado 
class Mesh {
private:
  std::vector<Vertex> m_verts; // vetor de vértices
  std::vector<Face> m_faces;   // vetor de vértices
  //  std::vector<Edge> m_edges;   // vetor de bordas (per ora, non usato)
  // construtor vazio. loadMesh deve ser usado neste caso. 
  Mesh();

  // Note: o entrelaçamento vai ser chamado pelo "m_env"
  // classe esta carregando o mesh
  void renderWire();
  void render(bool wireframe = false, bool gouraud_shading = true);

  // use os dois métodos a seguir para configurar o mesh
  void init();
  // bordas: coordenadas minimas e maximas
  void computeBoundingBox();
  void computeNormalsPerVertex();

public:
  // Funçao amiga  para carregar objeto intrelaçado invés de exportar
  friend std::unique_ptr<Mesh> loadMesh(const char *mesh_filename);
  Point3 bbmin, bbmax; // bordas

  // renderiza frontend 
  void renderFlat(bool wireframe = false);
  void renderGouraud(bool wireframe = false);

  // centro de eixos alinhados
  // Point3 center();
  Point3 center() { return (bbmin + bbmax) / 2.0; }
};

std::unique_ptr<Mesh> loadMesh(const char *mesh_filename);

using game::Key;        // chave padrao 
using game::MouseEvent; // chave padrao para eventos do mouse

class SmartWindow; // pre declaracao para ser usada no ambiente virtual

/* A classa Env representa o ambiente do jogo
    Manipula todos os componenetes principais da cena e todos callbacks
   Associado com os comandos.
*/
class Env final {

private:
  Env(); // constroi o ambiente, inicializando as variaveis.

  double m_fps;     // valor fps
  double m_fps_now; // fps atual
  uint m_last_time;
  int m_screenH, m_screenW;

  /* Callbacks:
   * eles serão o manipulador de eventos e renderização de teclas, mouse e janelas.
    * A função de callback real irá variar de acordo com o corrente estado do jogo.
    * Por exemplo, se estivermos no Menu, o retorno de chamada de renderização será diferente
    * de quando estiver rodando.
   */
  std::function<void()> m_action_handler, m_render_handler,
      m_window_event_handler;
  std::function<void(game::Key)> m_key_up_handler, m_key_down_handler;
  std::function<void(game::MouseEvent, int32_t, int32_t)> m_mouse_event_handler;

public:
  // expoes janelas de ambiente fora da classe
  bool m_wireframe, m_envmap, m_headlight, m_shadow, m_blending;

  // Amigos podem modificar partes privadas.
  friend Env &get_env();

  // destrutor fecha bibliotecas SDL libraries
  virtual ~Env();

  // accessadores
  inline decltype(m_wireframe) isWireframe() { return m_wireframe; }
  inline decltype(m_envmap) isEnvmap() { return m_envmap; }
  inline decltype(m_headlight) isHeadlight() { return m_headlight; }
  inline decltype(m_shadow) isShadow() { return m_shadow; }
  inline decltype(m_blending) isBlending() { return m_blending; }
  inline decltype(m_screenH) get_win_height() { return m_screenH; }
  inline decltype(m_screenW) get_win_width() { return m_screenW; }
  inline decltype(m_fps) get_fps() { return m_fps; }

  /*
    inline decltype(m_eye_dist) eyeDist() { return m_eye_dist; }
    inline decltype(m_view_alpha) alpha() {return m_view_alpha; }
    inline decltype(m_view_beta) beta() {return m_view_beta; } */

  inline void toggle_wireframe() { m_wireframe = !m_wireframe; }
  inline void toggle_envmap() { m_envmap = !m_envmap; }
  inline void toggle_headlight() { m_headlight = !m_headlight; }
  inline void toggle_shadow() { m_shadow = !m_shadow; }
  inline void toggle_blending() { m_blending = !m_blending; }

  // Setters callbacks
  // Default: vazio
  void set_action(decltype(m_action_handler) actions = [] {});
  void set_keydown_handler(decltype(m_key_down_handler) onkeydown = [](Key) {});
  void set_keyup_handler(decltype(m_key_up_handler) onkeyup = [](Key) {});
  void set_mouse_handler(decltype(m_mouse_event_handler) onmousev =
                             [](MouseEvent, int32_t, int32_t) {});
  void set_render(decltype(m_render_handler) render = [] {});
  void set_winevent_handler(decltype(m_window_event_handler) onwinev = [] {});

  // reseta variaveis do ambiente
  void reset();

  std::unique_ptr<SmartWindow> createWindow(std::string &name, size_t x,
                                            size_t y, size_t w, size_t h);
  void clearBuffer();
  void setColor(const Color &color);

  // funcoes de desenho 
  void drawCircle(double cx, double cy, double radius);
  void drawCubeFill(const float side);
  void drawCubeWire(const float side);
  void drawCube(const float side);
  void drawFloor(TexID texbind, float sz, float height, size_t num_quads);
  void drawPlane(float sz, float height, size_t num_quads);
  void drawPoint(double x, double y);
  void drawSky(TexID texbind, double radius, int lats, int longs);
  void drawSphere(double r, int lats, int longs);
  void drawSquare(const float side);
  void drawTorus(double r, double R);

  inline void disableLighting() { glDisable(GL_LIGHTING); }
  inline void enableLighting() { glEnable(GL_LIGHTING); }

  void enableDoubleBuffering();
  void enableVSync();
  void enableZbuffer(int depth);
  void enableJoystick();

  Uint32 getTicks();

  void lineWidth(float width);
  // Carrega a textura e retorna um boleano se der certo, deve se mudar para
  // retornar o ID da textura 
  TexID loadTexture(const char *filename, bool repeat = false,
                    bool nearest = false);

  // Recebe um lambda para transformar entre push and pop
  // Assegura que os eventos aconteçam em ordem
  void mat_scope(const std::function<void()> callback);

  /*
   * Funçao importante: main loop, executa até encontrar
   * SDL_Quit no evento até que despacha. 
   */

  // computa o FPS e renderiza!
  void render();
  void renderLoop();
  void quitLoop();

  void rotate(float angle, const Vec3 &axis);
  void scale(float scale_x, float scale_y, float scale_z);

  // configura a camera para mira a referencia (aim_x, aim_y, aim_z) do
  // frame observação (eye_x,y,z)
  void setCamera(double eye_x, double eye_y, double eye_z, double aim_x,
                 double aim_y, double aim_z, double upX, double upY,
                 double upZ);

  void setCoordToPixel();

  // Modela e projeta a matriz
  void setupModel();
  void setupPersp();

  // Configura as luzes
  void setupLightPosition();
  void setupModelLights();

  void translate(float scale_x, float scale_y, float scale_z);

  // desenha a testura
  void textureDrawing(TexID texbind, std::function<void()> callback,
                      bool gen_coordinates = true);
};

// returna isntancia do singleton 
Env &get_env();

/*
 * SmartWindow é basicamente uma classe que representa janela grafica, basicameente 
 * desdobra no topo da SDL_Window.
 */

class SmartWindow {

private:
  SDL_Window *m_win;         // janela SDL
  SDL_GLContext m_GLcontext; // Contexto SDL OpenGL 
  std::string m_name;        // nome da janela
  Env &m_env;

public:
  size_t m_width, m_height;

  SmartWindow(std::string &name, size_t x, size_t y, size_t w, size_t h);
  virtual ~SmartWindow();

  void hide();
  void refresh();
  void setupViewport();
  void show();
  void printOnScreen(std::function<void()> fn);
  void colorWindow(const Color &color);
  void textureWindow(TexID texbind);
};

/* __FONTS__
  * Uma biblioteca leve e rápida para carregar e usar TTF em OpenGL.
  * A única maneira de renderizar TTF no OpenGL é renderizar cada glifo como uma textura,
  * que, claro, carrega uma sobrecarga dolorosa no jogo em tempo de execução. o
  * solução é carregar um atlas de caracteres do TTF selecionado como um vetor de
  * texturas e armazená-lo na memória da GPU. Mais tarde, quando precisarmos processar o texto, vamos
  * basta apresentá-lo como uma lista de quadras texturizadas a partir de texturas pré-carregadas.
  */
// iniciando offset dos caracteres ASCII 
static const auto ASCII_SPACE_CODE = 0x20;
// finalizando offset dos caracteres ASCII 
static const auto ASCII_DEL_CODE = 0x7F;

// optimiza X GLubyte version of a Glyph
class Glyph {
private:
  // membros
  char m_letter; // freetype glyph index
  TexID m_texID;

  GLubyte m_minx;
  GLubyte m_miny;
  GLubyte m_maxx;
  GLubyte m_maxy;
  GLubyte m_advance; // numero de pixeis para avançar no eixo x

public:
  // accessor
  // Nota: accessors usam decltype como tipo de membro exato
  // pode ser alterado para otimização de memoria

  inline decltype(m_letter) get_letter() { return m_letter; }
  inline decltype(m_texID) get_textureID() { return m_texID; }
  inline decltype(m_advance) get_advance() { return m_advance; }
  inline decltype(m_minx) get_minX() { return m_minx; }
  inline decltype(m_miny) get_minY() { return m_miny; }
  inline decltype(m_maxx) get_maxX() { return m_maxx; }
  inline decltype(m_maxy) get_maxY() { return m_maxy; }

  Glyph(char letter, TexID textureID, GLubyte minx, GLubyte maxx, GLubyte miny,
        GLubyte maxy, GLubyte advance);
};

// Abstract GL TextRenderer
// Responsavel por carregar a TTF e inicializar a textura atlas vec.

class AGLTextRenderer {
private:
  // a textura do vetor atlas
  std::vector<Glyph> m_glyphs;
  int m_font_outline;
  int m_font_height;
  TTF_Font *m_font_ptr;
  Env &m_env; // cache do ambiente

  inline Glyph &get_glyph_at(size_t index) { return m_glyphs.at(index - ' '); }

  void loadTextureVector();
  
  // previne a chamada, chamando funçao comum
  AGLTextRenderer(const char *font_path, size_t font_size);
  void renderChar(int x_o, int y_o, char letter);

public:
  int render(int x_o, int y_o, const char *str);
  // mesma coisa que funcao acima mas para std::string
  int render(int x_o, int y_o, std::string &str);
  int renderf(int x_o, int y_o, const char *fmt, ...);
  int get_width(const char *str);

  inline decltype(m_font_height) get_height() { return m_font_height; }

  // sai em paz
  virtual ~AGLTextRenderer();
  // return singleton instance
  // friend AGLTextRenderer *getTextRenderer(const char *font_path,
  //                                         size_t font_size);
  friend std::unique_ptr<AGLTextRenderer> getTextRenderer(const char *font_path,
                                                          size_t font_size);
};

// AGLTextRenderer *getTextRenderer(const char *font_path, size_t font_size);
std::unique_ptr<AGLTextRenderer> getTextRenderer(const char *font_path,
                                                 size_t font_size);
} // namespace agl

#endif // AGL_H