#include "game.h"
#include "random"

namespace game {

Game::Game(std::string gameID, size_t num_rings)
    : m_gameID(gameID), m_state(State::SPLASH), 
      m_eye_dist(5.0), m_view_alpha(20.0), m_view_beta(40.0), m_victory(false),
      m_flappy3D(false), m_isFlappyOn(false), m_game_started(false), m_restart_game(false),
      m_deadline_time(0.0), m_last_time(.0),
      m_penalty_time(0.0), m_num_rings(num_rings), m_env(agl::get_env()),
      m_num_cubes(10), m_main_win(nullptr), m_floor(nullptr), m_sky(nullptr),
      m_final_door(nullptr), m_ssh(nullptr) {}

/*
 * Init the game:
 * 1. Obtain main window from the environment
 * 2. Load textures and mesh
 */
void Game::init() {

  std::string win_name = "Main Window";
  m_main_win = m_env.createWindow(win_name, 100, 0, m_env.get_win_width(),
                                  m_env.get_win_height());
  m_main_win->show();
  m_env.enableVSync();

  m_text_renderer = agl::getTextRenderer("fontes/neuropol.ttf", 30);
  m_text_big = agl::getTextRenderer("fontes/neuropol.ttf", 72);

  m_floor = elements::get_floor("texturas/sea.jpg");
  m_sky = elements::get_sky("texturas/space1.jpg");
  m_ssh = elements::get_spaceship("texturas/tex5.jpg", "objetos/Envos.obj",m_flappy3D);
  
  m_splash_tex = m_env.loadTexture("texturas/space.jpg");
  

  m_menu_tex = m_env.loadTexture("texturas/menu.jpg");
  init_rings();
  init_cubes();
 
}

void Game::changeState(game::State next_state) {
  static const auto TAG = __func__;

  if (next_state == m_state)
    return;

  switch (next_state) {
  case State::SPLASH:
    if ((!m_game_started) || (m_state == State::MENU && m_restart_game)) {
      m_state = next_state;
      restartGame();
      splash();
    }
    break;

  case State::MENU:
    if (m_state == State::GAME) {
      m_state = next_state;
      openSettings();
    }
    break;

  case State::GAME:
    if (m_state == State::SPLASH || m_state == State::MENU) {
      m_state = next_state;
      playGame();
    } else if (m_state == State::END && m_restart_game) {
      m_state = next_state;
      restartGame();
    }
    break;

  case State::END:
    if (m_state == State::GAME) {
      m_state = next_state;
      gameOver();
    }
    break;

  default:
    // shoudln't arrive here
    lg::e(TAG, "Game status not recognized");
  }
}

// victory: change state and save time for ranking
void Game::goToVictory() {
  lg::i(__func__, "GAME END!!");
  m_victory = true;
  m_player_time += (m_env.getTicks() - m_last_time);
  changeState(State::END);
}

// update all timings and check if deadline has come.
// If so, goes to gameover.
void Game::checkTime() {
  auto time_now = m_env.getTicks();
  auto diff = time_now - m_last_time;
  m_deadline_time -= diff;
  m_player_time += diff;
  // if a penalty has been triggered, compute its remaining time
  m_penalty_time = m_penalty_time > 0.0 ? (m_penalty_time - 100) : 0.0;
  m_last_time = time_now;

  if (m_deadline_time < 0) { // let's leave a last second hope
    m_victory = false;
    changeState(State::END);
  }
}

// check if current ring has been crossed 
// if so gives a bonus time + spawn next ring 
// if last ring has been crossed, go to victory 
void Game::checkRings() {
    // rings check
    auto &current_ring = m_rings.at(m_cur_ring_index);
    // check se gli anelli sono stati attraversati
    // spawn nuovo anello + bonus time || crea porta finale (time diventa rosso)
    current_ring.checkCrossing(m_ssh->x(), m_ssh->z());
    bool ring_crossed;
    ring_crossed = current_ring.isTriggered();

    if (ring_crossed) {
      auto bonus = m_flappy3D ? game::FLAPPY_RING_TIME : game::RING_TIME;
      m_deadline_time += bonus;
      m_cur_ring_index++;
      if (m_cur_ring_index >= m_num_rings) {
          goToVictory();
      }
    }
}

    // BadCubes check
// if the ship crosses one -> apply penalty
void Game::checkCubes() {
    for (auto &cube : m_cubes) {
      if (cube.checkCrossing(m_ssh->x(), m_ssh->z())) {
        lg::i(__func__, "Penalty!");
        m_penalty_time = 6000U;
        break;
      }
    }
}

void Game::gameAction() {
  // Game actions:
  // - Ship execute a step of physics
  // - time pass by
  // - if time is < 0 game over
  // - if crosses ring: get bonus time
  // - if ring is last one: final gate
  // - if crosses final gate: WIN!

  m_ssh->execute();

  // only if game has started, i.e. a key has been pressed
  if (m_game_started) {
    checkTime();
  }

  // if we are in final stage, only the final door is taken into account
  if (m_final_stage) {
    if (m_final_door->checkCrossing(m_ssh->x(), m_ssh->z())) {
      goToVictory();
    }
  }

  // else game proceeds normally
  else {
    checkCubes(); 
    checkRings(); 
  }
}

void Game::init_rings() {
  m_rings.clear();
  m_cur_ring_index = 0;

  // generate coordinate to place the rings using the coordinate generator
  // see coord_system.h
  // todo: add a check on minimum distance between each of the rings
  for (size_t i = 0; i < m_num_rings; ++i) {
    auto coords = coordinateGenerator::randomCoord3D();
    m_rings.emplace_back(coords.x, coords.y, coords.z, m_flappy3D);
  }
}

void Game::init_cubes() {
  // cubes
  m_cubes.clear();
  for (size_t i = 0; i < m_num_cubes; ++i) {
    auto coords = coordinateGenerator::randomCoord3D();
    m_cubes.emplace_back(coords.x, coords.y, coords.z, m_flappy3D);
  }
}



void Game::gameOnKey(Key key, bool pressed) {
  bool trig_motion = false;
  spaceship::Motion mt;

  switch (key) {

  case Key::W:
    mt = spaceship::Motion::THROTTLE;
    trig_motion = true;
    break;

  case Key::A:
    mt = spaceship::Motion::STEER_L;
    trig_motion = true;
    break;

  case Key::S:
    mt = spaceship::Motion::BRAKE;
    trig_motion = true;
    break;

  case Key::D:
    mt = spaceship::Motion::STEER_R;
    trig_motion = true;
    break;
  default:
    break;
  }

  // send a command to the spaceship only if triggered
  if (trig_motion) {
    if (!m_game_started) {
      m_game_started = true;
      m_last_time = m_env.getTicks();
      auto starting_time =
          m_flappy3D ? game::FLAPPY_RING_TIME : game::RING_TIME;
      m_deadline_time = starting_time;
    }

    m_ssh->sendCommand(mt, pressed);
  }
}


/* Esegue il Rendering della scena */
void Game::gameRender() {

  m_env.lineWidth(3.0);
  // remember to setup the viewport
  m_main_win->setupViewport();
  // buffer - lighting - perspective setup
  m_env.clearBuffer();
  m_env.disableLighting();
  m_env.setupPersp();
  m_env.setupModel();
  m_env.setupLightPosition();
  m_env.setupModelLights();
  // update camera
  setupShipCamera();

  // Render all elements
  m_floor->render();
  m_sky->render();

  // ---FLICKERING PENALTY---
  // if the spaceship hits a cube it will be rendered in a flickered way
  // switching from gouraud to wireframe rendering every 200ms
  if (m_penalty_time && ((m_penalty_time / 200) % 2 == 1)) {
    m_ssh->render(true);
  } else {
    m_ssh->render();
  }

  // rings: render till the first ring that's not triggered yet
  for (size_t i = 0; i < m_num_rings; ++i) {
    auto &ring = m_rings.at(i);
    ring.render();

    if (!ring.isTriggered()) {
      break;
    }
  }

  // render all BadCubes. They'll be an obstacle from the beginning
  for (auto &cube : m_cubes) {
    cube.render();
  }
  // apply shadow
  if (m_env.isShadow()) {
    m_ssh->shadow();
  }

  // HeadUp Display
  drawHUD();

  m_env.enableLighting();

  // refresh the view
  m_main_win->refresh();
}

/*
 * Bind all the callbacks of the environment to the corresponding
 * functions of the GAME state.
 */
void Game::playGame() {
  // Using placeholders to perform currying, together with std::bind()
  // Each placeholder (_1, _2, etc.) represents a future argument that will be
  // passed to the callback handler.
  using namespace std::placeholders;

  m_env.set_winevent_handler(std::bind(&Game::gameRender, this));
  m_env.set_render(std::bind(&Game::gameRender, this));
  m_env.set_action(std::bind(&Game::gameAction, this));

  m_env.set_keydown_handler(std::bind(&Game::gameOnKey, this, _1, true));
  m_env.set_keyup_handler(std::bind(&Game::gameOnKey, this, _1, false));
 
}

void Game::restartGame() {
  static const auto TAG = __func__;

  lg::i(TAG, "Starting NEW game...");
  // game vars
  m_restart_game = m_game_started = false;
  m_player_time = m_deadline_time = 0.0;
  m_penalty_time = m_last_time = 0;

  m_env.reset();

  // elements
  // handle 3D flight if activated
  // retrieve the right type of ship 
  m_ssh = elements::get_spaceship("texturas/tex5.jpg", "objetos/Envos.obj",
                                    m_flappy3D);
  m_isFlappyOn = m_flappy3D; // flag to remember the current game is in flappy mode

  init_rings();
  init_cubes();
  
  playGame();
}

/*
 * Run the game.
 * 1. Init; 2. Splash screen; 3. Main event loop
 */
void Game::run() {
  init();

  splash();

  m_env.renderLoop();
}

void Game::setupShipCamera() {
  // angle of the SShip wrt to X-axis in the x0z plane
  double angle = m_ssh->facing();
  double cosf = cos(angle * M_PI / 180.0);
  double sinf = sin(angle * M_PI / 180.0);
  double cam_d, cam_h, eye_x, eye_y, eye_z, cen_x, cen_y, cen_z;
  double cosff, sinff;

  float px = m_ssh->x();
  float py = m_ssh->y();
  float pz = m_ssh->z();
    
  //Define a camera back para SHIP
  cam_d = 2.3;
  cam_h = 1.0;
  eye_x = px + cam_d * sinf;
  eye_y = py + cam_h;
  eye_z = pz + cam_d * cosf;
  cen_x = px - cam_d * sinf;
  cen_y = py + cam_h;
  cen_z = pz - cam_d * cosf;
  m_env.setCamera(eye_x, eye_y, eye_z, cen_x, cen_y, cen_z, 0.0, 1.0, 0.0);
 
}
} // namespace game
