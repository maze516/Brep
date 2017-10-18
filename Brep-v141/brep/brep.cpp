#include "brep.hpp"

#include <assert.h>
#include <Windows.h>
#include <gl/GL.h>
#include <gl/GLU.h>
#include <fstream>

#include "brep_struct.hpp"

void Brep::mvfs(float x, float y, float z)
{
    BSolid *solid = new BSolid();
    BFace *face = new BFace();
    BLoop *loop = new BLoop();
    BHalfEdge *halfEdge = new BHalfEdge();
    BVertex *vertex = new BVertex(x, y, z);

    solids.push_back(solid);

    solid->faces.push_back(face);
    face->solid = solid;

    face->outLoop = loop;
    face->loops.push_back(loop);
    loop->face = face;

    loop->firstHalfEdge = halfEdge;
    halfEdge->loop = loop;
    halfEdge->prev = halfEdge;
    halfEdge->next = halfEdge;

    halfEdge->vertex = vertex;
    halfEdge->edge = nullptr;
    vertex->halfEdge = halfEdge;
    solid->vertices.push_back(vertex);
}

BHalfEdge *Brep::mev(BLoop *loop, BVertex *vertex, float x, float y, float z)
{
    BEdge *e = new BEdge();
    BHalfEdge *he = nullptr;
    BHalfEdge *he1 = nullptr;
    BHalfEdge *he2 = new BHalfEdge();
    BVertex *v = new BVertex(x, y, z);

    /* he1 may exists already */
    he = loop->firstHalfEdge;
    do
    {
        if (he->vertex == vertex)
            break;
        he = he->next;
    } while (he != loop->firstHalfEdge);
    assert(he->vertex == vertex);

    loop->face->solid->vertices.push_back(v);
    /* Virtual Half Edge(The first Half Edge of the Edge already exists) */
    if (he->edge == nullptr)
    {
        he1 = he;
        he1->next = he2;
        he1->prev = he2;
        he2->next = he1;
        he2->prev = he1;

        he2->loop = loop;
        he2->vertex = v;
        he1->edge = e;
        he2->edge = e;
    }
    /* Real Half Edge(Need to create new Half Edge) */
    else
    {
        he1 = new BHalfEdge();
        he1->next = he2;
        he1->prev = he->prev;
        he2->next = he;
        he2->prev = he1;
        he->prev->next = he1;
        he->prev = he2;

        he1->loop = loop;
        he2->loop = loop;
        he1->vertex = vertex;
        he2->vertex = v;
        he1->edge = e;
        he2->edge = e;
    }
    e->halfEdgeA = he1;
    e->halfEdgeB = he2;
    loop->face->solid->edges.push_back(e);

    return he1;
}

void Brep::mef(BLoop *outLoop, BVertex *vertex1, BVertex *vertex2)
{
    BHalfEdge *he1 = outLoop->findHalfEdgeWithVertex(vertex1);
    BHalfEdge *he2 = outLoop->findHalfEdgeWithVertex(vertex2);

    BFace *face = new BFace();
    BLoop *loop = new BLoop();
    BEdge *edge = new BEdge();
    BHalfEdge *nhe1 = new BHalfEdge();
    BHalfEdge *nhe2 = new BHalfEdge();

    outLoop->face->solid->faces.push_back(face);
    face->solid = outLoop->face->solid;

    face->loops.push_back(loop);
    face->outLoop = loop;
    loop->face = face;

    outLoop->firstHalfEdge = nhe1;
    loop->firstHalfEdge = nhe2;
    /* Move half edges with inner normal to new loop */
    for (BHalfEdge *he = he1; he != he2; he = he->next)
    {
        he->loop = loop;
    }

    nhe1->prev = he1->prev;
    nhe1->next = he2;
    nhe2->prev = he2->prev;
    nhe2->next = he1;
    nhe1->edge = edge;
    nhe2->edge = edge;
    nhe1->vertex = vertex1;
    nhe2->vertex = vertex2;
    nhe1->loop = outLoop;
    nhe2->loop = loop;

    edge->halfEdgeA = nhe1;
    edge->halfEdgeB = nhe2;
    outLoop->face->solid->edges.push_back(edge);

    he1->prev->next = nhe1;
    he1->prev = nhe2;
    he2->prev->next = nhe2;
    he2->prev = nhe1;
}

void Brep::dump()
{
    std::ofstream dumpFile("brep_dump.txt");
    std::list<BSolid *>::iterator solidIt = solids.begin();
    int solidId = 0;
    for (; solidIt != solids.end(); ++solidIt, ++solidId)
    {
        BSolid *solid = *solidIt;
        dumpFile << "Solid" << solidId << " | " << solid << "\n";
        // Faces
        std::list<BFace *>::iterator faceIt = solid->faces.begin();
        int faceId = 0;
        for (; faceIt != solid->faces.end(); ++faceIt, ++faceId)
        {
            dumpFile << "  Face" << faceId << "\n";
            BFace *face = *faceIt;
            dumpFile << "    OutLoop: " << face->outLoop << "\n";
            std::list<BLoop *>::iterator loopIt = face->loops.begin();
            int loopId = 0;
            for (; loopIt != face->loops.end(); ++loopIt, ++loopId)
            {
                dumpFile << "    Loop" << loopId << "\n";
                BLoop *loop = *loopIt;
                BHalfEdge *he = loop->firstHalfEdge;
                do
                {
                    dumpFile << "      HalfEdge: " << he << "\n";
                    he = he->next;
                } while (he != loop->firstHalfEdge);
            }
        }

        // Edges
        std::list<BEdge *>::iterator edgeIt = solid->edges.begin();
        int edgeId = 0;
        for (; edgeIt != solid->edges.end(); ++edgeIt, ++edgeId)
        {
            BEdge *edge = *edgeIt;
            dumpFile << "  Edge" << edgeId << " | " << edge << "\n";
            dumpFile << "    HalfEdgeA: " << edge->halfEdgeA << " V: " << edge->halfEdgeA->vertex << "\n";
            dumpFile << "    HalfEdgeB: " << edge->halfEdgeB << " V: " << edge->halfEdgeB->vertex << "\n";
        }

        // Vertices
        std::list<BVertex *>::iterator vertexIt = solid->vertices.begin();
        int vertexId = 0;
        for (; vertexIt != solid->vertices.end(); ++vertexIt, ++vertexId)
        {
            BVertex *vertex = *vertexIt;
            dumpFile << "  Vertex" << vertexId << " | " << vertex << "(";
            dumpFile << vertex->x << ", " << vertex->y << ", " << vertex->z << ")\n";
        }
    }
    dumpFile.close();

    //int main()
    //{
    //    Brep brep;
    //    brep.mvfs(0, 0, 0);
    //    brep.mev(brep.solids.front()->faces.front()->outLoop, brep.solids.front()->vertices.back(), 1, 0, 0);
    //    brep.mev(brep.solids.front()->faces.front()->outLoop, brep.solids.front()->vertices.back(), 1, -1, 0);
    //    brep.mev(brep.solids.front()->faces.front()->outLoop, brep.solids.front()->vertices.back(), 0, -1, 0);
    //}
}
