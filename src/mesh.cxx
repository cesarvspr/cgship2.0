#include "agl.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

#include <cstdio>
#include <cstring>

/*
 * Class implementing a Mesh. See agl.h
 */

namespace agl {
// Face constructor, compute the normal of the face too
Face::Face(Vertex *v1, Vertex *v2, Vertex *v3) : verts{v1, v2, v3} {
  computeNormal();
}

Mesh::Mesh() {}

// Computo normali per vertice
// (come media rinormalizzata delle normali delle facce adjacenti)
void Mesh::computeNormalsPerVertex() {
  // uso solo le strutture di navigazione FV (da Faccia a Vertice)!

  // fase uno: ciclo sui vertici, azzero tutte le normali
  for (auto &vertex : m_verts) {
    vertex.normal = Normal3();
  }

  // fase due: ciclo sulle facce: accumulo le normali di F nei 3 V
  // corrispondenti
  for (auto &face : m_faces) {
    for (auto vertex : face.verts) {
      vertex->normal += face.normal;
    }
    /*
    f[i].v[0]->n = f[i].v[0]->n + f[i].n;
    f[i].v[1]->n = f[i].v[1]->n + f[i].n;
    f[i].v[2]->n = f[i].v[2]->n + f[i].n;
    */
  }

  // fase tre: ciclo sui vertici e rinormalizzo:
  // la normale media rinormalizzata e' uguale alla somma delle normnali,
  // calcolata nel ciclo precedente, ma rinormalizzata
  for (auto &vertex : m_verts) {
    vertex.normal = vertex.normal.normalize();
  }
}

// renderizzo la mesh in wireframe
void Mesh::renderWire() {
  glLineWidth(1.0);
  // (nota: ogni edge viene disegnato due volte.
  // sarebbe meglio avere ed usare la struttura edge)
  glBegin(GL_LINE_LOOP);
  for (auto &face : m_faces) {
    face.normal.render();
    for (auto vertex : face.verts) {
      // render as vertex, don't send the normal
      vertex->render(false);
    }
  }
  glEnd();

  /*  {
      glBegin(GL_LINE_LOOP);
      f[i].n.getAsNormal();
      (f[i].v[0])->p.getAsVertex();
      (f[i].v[1])->p.getAsVertex();
      (f[i].v[2])->p.getAsVertex();
      glEnd();
    }*/
}

// Render usando la normale per faccia (FLAT SHADING)
void Mesh::renderFlat(bool wireframe_on) { render(wireframe_on, false); }

void Mesh::renderGouraud(bool wireframe_on) { render(wireframe_on, true); }

// Render usando la normale per vertice (GOURAUD SHADING)
void Mesh::render(bool wireframe_on, bool goraud_shading) {
  if (wireframe_on) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(.5, .5, .5);
    renderWire();
    glColor3f(1, 1, 1);
  }

  glBegin(GL_TRIANGLES);

  if (goraud_shading) {
    // mandiamo tutti i triangoli a schermo
    bool send_normals = goraud_shading;

    for (const auto &face : m_faces) {
      // If using flat shading
      if (!send_normals) {
        face.normal.render();
      }

      for (auto vert : face.verts) {
        vert->render(send_normals);
      }
    }
  }
  /*
        for (int i = 0; i < f.size(); i++) {
          (f[i].v[0])->n.getAsNormal();
          (f[i].v[0])->p.getAsVertex();

          (f[i].v[1])->n.getAsNormal();
          (f[i].v[1])->p.getAsVertex();

          (f[i].v[2])->n.getAsNormal();
          (f[i].v[2])->p.getAsVertex();
        }
    }
      else {
        //flat shading
        for (int i = 0; i < f.size(); i++) {
          f[i].n.getAsNormal(); // flat shading

          (f[i].v[0])->p.getAsVertex();

          (f[i].v[1])->p.getAsVertex();

          (f[i].v[2])->p.getAsVertex();
        }
    }
  */
  glEnd();
}

// trova l'AXIS ALIGNED BOUNDIG BOX
void Mesh::computeBoundingBox() {
  // basta trovare la min x, y, e z, e la max x, y, e z di tutti i vertici
  // (nota: non e' necessario usare le facce: perche?)
  // init var to worse value
  float min_x, min_y, min_z = INFINITY;
  float max_x, max_y, max_z = -INFINITY;

  // find maximum and minimum among vertices
  for (const auto &vertex : m_verts) {
    min_x = std::min(min_x, vertex.point.x);
    min_y = std::min(min_y, vertex.point.y);
    min_z = std::min(min_z, vertex.point.z);

    max_x = std::max(max_x, vertex.point.x);
    max_y = std::max(max_y, vertex.point.y);
    max_z = std::max(max_z, vertex.point.z);
  }

  bbmin = Point3(min_x, min_y, min_z);
  bbmax = Point3(max_x, max_y, max_z);
}

// Point3 Mesh::center() { return (bbmin + bbmax) / 2.0; };

// init vertex normals and bounding box
void Mesh::init() {
  computeNormalsPerVertex();
  computeBoundingBox();
}

//   carica la mesh da un file in formato Obj
//   Nota: nel file, possono essere presenti sia quads che tris
//   ma nella rappresentazione interna (classe Mesh) abbiamo solo tris.

// Friend class, must be used instead of the constructor
std::unique_ptr<Mesh> loadMesh(const char *filename) {
  static const auto TAG = __func__;

  lg::i(TAG, "Loading mesh from file %s", filename);

  std::unique_ptr<Mesh> ret(new Mesh());

  FILE *file = fopen(filename, "rt"); // apriamo il file in lettura
  if (!file) {
    // whenever I've spare time, an exception class must be added
    lg::e(TAG, "Cannot load mesh from %s", filename);
    exit(EXIT_FAILURE);
  }

  // make a first pass through the file to get a count of the number
  // of vertices, normals, texcoords & triangles
  char buf[128];
  int nv, nf, nt;
  float x, y, z;
  int va, vb, vc, vd;
  int na, nb, nc, nd;
  int ta, tb, tc, td;

  nv = 0;
  nf = 0;
  nt = 0;
  while (std::fscanf(file, "%s", buf) != EOF) {
    switch (buf[0]) {
    case '#': // comment
      // eat up rest of line
      std::fgets(buf, sizeof(buf), file);
      break;
    case 'v': // v, vn, vt
      switch (buf[1]) {
      case '\0': // vertex
        // eat up rest of line
        std::fgets(buf, sizeof(buf), file);
        nv++;
        break;
      default:
        break;
      }
      break;
    case 'f': // face
      std::fscanf(file, "%s", buf);
      // can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d
      if (std::strstr(buf, "//")) {
        // v//n
        std::sscanf(buf, "%d//%d", &va, &na);
        std::fscanf(file, "%d//%d", &vb, &nb);
        std::fscanf(file, "%d//%d", &vc, &nc);
        nf++;
        nt++;
        while (std::fscanf(file, "%d//%d", &vd, &nd) > 0) {
          nt++;
        }
      } else if (std::sscanf(buf, "%d/%d/%d", &va, &ta, &na) == 3) {
        // v/t/n
        std::fscanf(file, "%d/%d/%d", &vb, &tb, &nb);
        std::fscanf(file, "%d/%d/%d", &vc, &tc, &nc);
        nf++;
        nt++;
        while (std::fscanf(file, "%d/%d/%d", &vd, &td, &nd) > 0) {
          nt++;
        }
      } else if (std::sscanf(buf, "%d/%d", &va, &ta) == 2) {
        // v/t
        std::fscanf(file, "%d/%d", &vb, &tb);
        std::fscanf(file, "%d/%d", &vc, &tc);
        nf++;
        nt++;
        while (std::fscanf(file, "%d/%d", &vd, &td) > 0) {
          nt++;
        }
      } else {
        // v
        std::fscanf(file, "%d", &va);
        std::fscanf(file, "%d", &vb);
        nf++;
        nt++;
        while (std::fscanf(file, "%d", &vc) > 0) {
          nt++;
        }
      }
      break;

    default:
      // eat up rest of line
      std::fgets(buf, sizeof(buf), file);
      break;
    }
  }

  // printf("dopo FirstPass nv=%d nf=%d nt=%d\n",nv,nf,nt);

  // allochiamo spazio per nv vertici
  ret->m_verts.resize(nv);

  // rewind to beginning of file and read in the data this pass
  rewind(file);

  // on the second pass through the file, read all the data into the
  // allocated arrays

  nv = 0;
  nt = 0;
  while (std::fscanf(file, "%s", buf) != EOF) {
    switch (buf[0]) {
    case '#': // comment
      // eat up rest of line
      std::fgets(buf, sizeof(buf), file);
      break;
    case 'v': // v, vn, vt
      switch (buf[1]) {
      case '\0': // vertex
        std::fscanf(file, "%f %f %f", &x, &y, &z);
        ret->m_verts[nv] = Point3(x, y, z);
        nv++;
        break;
      default:
        break;
      }
      break;
    case 'f': // face
      std::fscanf(file, "%s", buf);
      // can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d
      if (std::strstr(buf, "//")) {
        // v//n
        std::sscanf(buf, "%d//%d", &va, &na);
        std::fscanf(file, "%d//%d", &vb, &nb);
        std::fscanf(file, "%d//%d", &vc, &nc);
        va--;
        vb--;
        vc--;

        // create on place a new face and save it at the end of the face
        // vector
        ret->m_faces.emplace_back(&(ret->m_verts[va]), &(ret->m_verts[vc]),
                                  &(ret->m_verts[vb]));

        nt++;
        vb = vc;
        while (std::fscanf(file, "%d//%d", &vc, &nc) > 0) {
          vc--;
          ret->m_faces.emplace_back(&(ret->m_verts[va]), &(ret->m_verts[vc]),
                                    &(ret->m_verts[vb]));

          nt++;
          vb = vc;
        }
      } else if (std::sscanf(buf, "%d/%d/%d", &va, &ta, &na) == 3) {
        // v/t/n
        std::fscanf(file, "%d/%d/%d", &vb, &tb, &nb);
        std::fscanf(file, "%d/%d/%d", &vc, &tc, &nc);
        va--;
        vb--;
        vc--;

        ret->m_faces.emplace_back(&(ret->m_verts[va]), &(ret->m_verts[vc]),
                                  &(ret->m_verts[vb]));

        nt++;
        vb = vc;
        while (std::fscanf(file, "%d/%d/%d", &vc, &tc, &nc) > 0) {
          vc--;

          ret->m_faces.emplace_back(&(ret->m_verts[va]), &(ret->m_verts[vc]),
                                    &(ret->m_verts[vb]));

          nt++;
          vb = vc;
        }
      } else if (std::sscanf(buf, "%d/%d", &va, &ta) == 2) {
        // v/t
        std::fscanf(file, "%d/%d", &va, &ta);
        std::fscanf(file, "%d/%d", &va, &ta);
        va--;
        vb--;
        vc--;

        ret->m_faces.emplace_back(&(ret->m_verts[va]), &(ret->m_verts[vc]),
                                  &(ret->m_verts[vb]));

        nt++;
        vb = vc;
        while (std::fscanf(file, "%d/%d", &vc, &tc) > 0) {
          vc--;

          ret->m_faces.emplace_back(&(ret->m_verts[va]), &(ret->m_verts[vc]),
                                    &(ret->m_verts[vb]));

          nt++;
          vb = vc;
        }
      } else {
        // v
        std::sscanf(buf, "%d", &va);
        std::fscanf(file, "%d", &vb);
        std::fscanf(file, "%d", &vc);
        va--;
        vb--;
        vc--;

        ret->m_faces.emplace_back(&(ret->m_verts[va]), &(ret->m_verts[vc]),
                                  &(ret->m_verts[vb]));

        nt++;
        vb = vc;
        while (std::fscanf(file, "%d", &vc) > 0) {
          vc--;

          ret->m_faces.emplace_back(&(ret->m_verts[va]), &(ret->m_verts[vc]),
                                    &(ret->m_verts[vb]));

          nt++;
          vb = vc;
        }
      }
      break;

    default:
      // eat up rest of line
      std::fgets(buf, sizeof(buf), file);
      break;
    }
  }

  // printf("dopo SecondPass nv=%d nt=%d\n",nv,nt);

  std::fclose(file);

  // compute vertex normals and BB
  ret->init();
  ret->computeBoundingBox();

  return ret;
}
} // namespace agl
