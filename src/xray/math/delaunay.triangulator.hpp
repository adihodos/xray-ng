// MIT License
//
// DELABELLA - Delaunay triangulation library
// Copyright (C) 2018 GUMIX - Marcin Sokalski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cmath>
#include <cstdint>

#define FP_FAST_FMAF
#define FP_FAST_FMA
#define FP_FAST_FMAL
#include "xray/math/predicates.h"

template<typename T = double, typename I = int>
struct IDelaBella2
{
    struct Vertex;
    struct Simplex;
    struct Iterator;

    struct Vertex
    {
        static const T resulterrbound;
        Vertex* next; // next in internal / boundary set of vertices
        Simplex* sew; // one of triangles sharing this vertex
        T x, y;       // coordinates (input copy)
        I i;          // index of original point

        static bool overlap(const Vertex* v1, const Vertex* v2) { return v1->x == v2->x && v1->y == v2->y; }
        inline const Simplex* StartIterator(Iterator* it /*not_null*/) const;
    };

    struct Simplex
    {
        static const T iccerrboundA;

        Vertex* v[3];  // 3 vertices spanning this triangle
        Simplex* f[3]; // 3 adjacent faces, f[i] is at the edge opposite to vertex v[i]
        Simplex* next; // next triangle (of delaunay set or hull set)
        I index;       // list index

        unsigned char flags;

        bool IsDelaunay() const { return !(flags & 0b10000000); }

        bool IsInterior(int at) const { return flags & 0b01000000; }

        bool IsEdgeFixed(int at) const { return flags & (1 << at); }

        inline const Simplex* StartIterator(Iterator* it /*not_null*/, int around /*0,1,2*/) const;

        void RotateEdgeFlagsCCW() // <<
        {
            uint8_t f = this->flags;
            this->flags = ((f << 1) & 0b00110110) | ((f >> 2) & 0b00001001) | (f & 0b11000000);
        }

        void RotateEdgeFlagsCW() // >>
        {
            uint8_t f = this->flags;
            this->flags = ((f >> 1) & 0b00011011) | ((f << 2) & 0b00100100) | (f & 0b11000000);
        }

        void ToggleEdgeFixed(int at) { this->flags = (this->flags ^ (0b00001000 << at)) | (0b00000001 << at); }

        uint8_t GetEdgeBits(int at) const { return (this->flags >> at) & 0b00001001; }

        void SetEdgeBits(int at, uint8_t bits) { this->flags = (bits << at) | (this->flags & ~(0b00001001 << at)); }

        static Simplex* Alloc(Simplex** from)
        {
            Simplex* f = *from;
            *from = (Simplex*)f->next;
            f->next = 0;
            return f;
        }

        void Free(Simplex** to)
        {
            this->next = *to;
            *to = this;
        }

        Simplex* Next(const Vertex* p) const
        {
            if (this->v[0] == p)
                return (Simplex*)this->f[1];
            if (this->v[1] == p)
                return (Simplex*)this->f[2];
            if (this->v[2] == p)
                return (Simplex*)this->f[0];
            return 0;
        }

        bool signN() const
        {
            return 0 > predicates::adaptive::orient2d(
                           this->v[0]->x, this->v[0]->y, this->v[1]->x, this->v[1]->y, this->v[2]->x, this->v[2]->y);
        }

        bool sign0() const
        {
            return 0 == predicates::adaptive::orient2d(
                            this->v[0]->x, this->v[0]->y, this->v[1]->x, this->v[1]->y, this->v[2]->x, this->v[2]->y);
        }

        bool dot0(const Vertex& p) const
        {
            return predicates::adaptive::incircle(p.x,
                                                  p.y,
                                                  this->v[0]->x,
                                                  this->v[0]->y,
                                                  this->v[1]->x,
                                                  this->v[1]->y,
                                                  this->v[2]->x,
                                                  this->v[2]->y) == 0;
        }

        bool dotP(const Vertex& p) const
        {
            return predicates::adaptive::incircle(p.x,
                                                  p.y,
                                                  this->v[0]->x,
                                                  this->v[0]->y,
                                                  this->v[1]->x,
                                                  this->v[1]->y,
                                                  this->v[2]->x,
                                                  this->v[2]->y) > 0;
        }

        bool dotN(const Vertex& p) const
        {
            return predicates::adaptive::incircle(p.x,
                                                  p.y,
                                                  this->v[0]->x,
                                                  this->v[0]->y,
                                                  this->v[1]->x,
                                                  this->v[1]->y,
                                                  this->v[2]->x,
                                                  this->v[2]->y) < 0;
        }

        bool dotNP(const Vertex& p) /*const*/
        {
            { // somewhat faster, poor compiler inlining?

                const T dx = this->v[2]->x;
                const T dy = this->v[2]->y;

                const T adx = p.x - dx;
                const T ady = p.y - dy;
                const T bdx = this->v[0]->x - dx;
                const T bdy = this->v[0]->y - dy;
                const T cdx = this->v[1]->x - dx;
                const T cdy = this->v[1]->y - dy;

                const T adxcdy = adx * cdy;
                const T adxbdy = adx * bdy;
                const T bdxcdy = bdx * cdy;
                const T bdxady = bdx * ady;
                const T cdxbdy = cdx * bdy;
                const T cdxady = cdx * ady;

                const T alift = adx * adx + ady * ady;
                const T blift = bdx * bdx + bdy * bdy;
                const T clift = cdx * cdx + cdy * cdy;

                const T dif_bdxcdy_cdxbdy = bdxcdy - cdxbdy;
                const T sum_abs_bdxcdy_cdxbdy = std::abs(bdxcdy) + std::abs(cdxbdy);

                const T det_a = alift * dif_bdxcdy_cdxbdy;
                const T det_b = blift * (cdxady - adxcdy);
                const T det_c = clift * (adxbdy - bdxady);

                const T det = det_a + det_b + det_c;

                const T permanent = sum_abs_bdxcdy_cdxbdy * alift + (std::abs(cdxady) + std::abs(adxcdy)) * blift +
                                    (std::abs(adxbdy) + std::abs(bdxady)) * clift;

                T errbound = iccerrboundA * permanent;
                if (std::abs(det) >= std::abs(errbound))
                    return det <= 0;
            }

            return predicates::adaptive::incircle(p.x,
                                                  p.y,
                                                  this->v[0]->x,
                                                  this->v[0]->y,
                                                  this->v[1]->x,
                                                  this->v[1]->y,
                                                  this->v[2]->x,
                                                  this->v[2]->y) <= 0;
        }
    };

    struct Iterator
    {
        const Simplex* current;
        int around;

        const Simplex* Next()
        {
            int pivot = around + 1;
            if (pivot == 3)
                pivot = 0;

            Simplex* next = current->f[pivot];
            Vertex* v = current->v[around];

            if (next->v[0] == v)
                around = 0;
            else if (next->v[1] == v)
                around = 1;
            else
                around = 2;

            current = next;
            return current;
        }

        const Simplex* Prev()
        {
            int pivot = around - 1;
            if (pivot == -1)
                pivot = 2;

            Simplex* prev = current->f[pivot];
            Vertex* v = current->v[around];

            if (prev->v[0] == v)
                around = 0;
            else if (prev->v[1] == v)
                around = 1;
            else
                around = 2;

            current = prev;
            return current;
        }
    };

    Vertex* vert_alloc{};
    Simplex** face_alloc{};
    I* vert_map{};
    I max_verts{};
    I max_faces{};

    Simplex** first_dela_face{};
    Simplex** first_hull_face{};
    Vertex* first_boundary_vert{};
    Vertex* first_internal_vert{};

    I inp_verts{};
    I out_verts{};
    I polygons{};
    I out_hull_faces{};
    I out_boundary_verts{};
    I unique_points{};

    T trans[2]; // experimental

    virtual void SetErrLog(int (*proc)(void* stream, const char* fmt, ...), void* stream) = 0;

    // return 0: no output
    // negative: all points are colinear, output hull vertices form colinear segment list, no triangles on output
    // positive: output hull vertices form counter-clockwise ordered segment contour, delaunay and hull triangles are
    // available if 'y' pointer is null, y coords are treated to be located immediately after every x if advance_bytes
    // is less than 2*sizeof coordinate type, it is treated as 2*sizeof coordinate type
    I Triangulate(I points, const T* x, const T* y = 0, size_t advance_bytes = 0, I stop = -1) = 0;

    // num of points passed to last call to Triangulate()
    I GetNumInputPoints() const = 0;

    // num of indices returned from last call to Triangulate()
    I GetNumOutputIndices() const = 0;

    // num of hull faces (non delaunay triangles)
    I GetNumOutputHullFaces() const = 0;

    // num of boundary vertices
    I GetNumBoundaryVerts() const = 0;

    // num of internal vertices
    I GetNumInternalVerts() const = 0;

    // when called right after Triangulate() / Constrain() / FloodFill() it returns number of triangles,
    // but if called after Polygonize() it returns number of polygons
    I GetNumPolygons() const = 0;

    const Simplex* GetFirstDelaunaySimplex() const = 0; // valid only if Triangulate() > 0
    const Simplex* GetFirstHullSimplex() const = 0;     // valid only if Triangulate() > 0
    const Vertex* GetFirstBoundaryVertex() const = 0;   // if Triangulate() < 0 it is list, otherwise closed contour!
    const Vertex* GetFirstInternalVertex() const = 0;   // valid only if Triangulate() > 0

    // given input point index, returns corresponding vertex pointer
    const Vertex* GetVertexByIndex(I i) const = 0;

    // insert constraint edges into triangulation, valid only if Triangulate() > 0
    I ConstrainEdges(I edges, const I* pa, const I* pb, size_t advance_bytes) = 0;

    // assigns interior / exterior flags to all faces, valid only if Triangulate() > 0
    // returns number of 'land' faces (they start at GetFirstDelaunaySimplex)
    // optionally <exterior> pointer is set to the first 'sea' face
    // if invert is set, outer-most faces will become 'land' (instead of 'sea')
    // depth controls how many times (0=INF) wave front can pass through constraints rings
    I FloodFill(bool invert, const Simplex** exterior = 0, int depth = 0) = 0;

    // groups adjacent faces, not separated by constraint edges, built on concyclic vertices into polygons
    // first 3 vertices of a polygon are all 3 vertices of first face Simplex::v[0], v[1], v[2]
    // every next face in polygon defines additional 1 polygon vertex at its Simplex::v[0]
    // usefull as preprocessing step before genereating voronoi diagrams
    // and as unification step before comparing 2 or more triangulations
    // valid only if Triangulate() > 0
    I Polygonize(const Simplex* poly[/*GetNumOutputIndices()/3*/] = 0) = 0;

    // GenVoronoiDiagramVerts(), valid only if Triangulate() > 0
    // it makes sense to call it prior to constraining only
    // generates VD vertices (for use with VD edges or VD polys indices)
    // <x>,<y> can be null if only number of vertices is needed
    // assuming:
    //   N = GetNumBoundaryVerts()
    //   M = GetNumInternalVerts()
    //   P = GetNumPolygons()
    //   V = P + N
    // <x>,<y> array must be (at least) V elements long
    // first P <x>,<y> elements will contain internal points (divisor W=1)
    // next N <x>,<y> elements will contain edge normals (divisor W=0)
    // function returns number vertices filled (V) on success, otherwise 0
    I GenVoronoiDiagramVerts(T* x, T* y, size_t advance_bytes = 0) const = 0;

    // GenVoronoiDiagramEdges(), valid only if Triangulate() > 0
    // it makes sense to call it prior to constraining only
    // generates unidirected VD edges (without ones in opposite direction)
    // assuming:
    //   N = GetNumBoundaryVerts()
    //   M = GetNumInternalVerts()
    //   P = GetNumPolygons()
    //   I = 2 * (N + M + P - 1)
    // <indices> must be (at least) I elements long or must be null
    // every pair of consecutive values in <indices> represent VD edge
    // there is no guaranteed correspondence between edges order and other data
    // function returns number of indices filled (I) on success, otherwise 0
    I GenVoronoiDiagramEdges(I* indices, size_t advance_bytes = 0) const = 0;

    // GenVoronoiDiagramPolys() valid only if Triangulate() > 0
    // it makes sense to call it prior to constraining only
    // generates VD polygons
    // assuming:
    //   N = GetNumBoundaryVerts()
    //   M = GetNumInternalVerts()
    //   P = GetNumPolygons()
    //   I = 3 * (N + M) + 2 * (P - 1) + N
    // <indices> must be (at least) I elements long or must be null
    // first M polys in <indices> represent closed VD cells, thay are in order
    // and corresponding to vertices from GetFirstInternalVertex() -> Vertex::next list
    // next N polys in <indices> represent open VD cells, thay are in order
    // and corresponding to vertices from GetFirstBoundaryVertex() -> Vertex::next list
    // every poly written to <indices> is terminated with ~0 value
    // if both <indices> and <closed_indices> are not null,
    // number of closed VD cells indices is written to <closed_indices>
    // function returns number of indices filled (I) on success, otherwise 0
    I GenVoronoiDiagramPolys(I* indices, size_t advance_bytes = 0, I* closed_indices = 0) const = 0;

    void CheckTopology() const = 0;

  private:
    bool ReallocVerts(I points);
    I Prepare(I* start, Simplex** hull, I* out_hull_faces, Simplex** cache, uint64_t* sort_stamp, I stop);
	Simplex* FindConstraintOffenders(Vertex* va, Vertex* vb, Simplex*** ptail, Vertex** restart);
	bool FindEdgeFaces(const Vertex* v1, const Vertex* v2, Simplex* twin[2], int opposed[2]);
};

template<typename T, typename I>
inline const typename IDelaBella2<T, I>::Simplex*
IDelaBella2<T, I>::Simplex::StartIterator(IDelaBella2<T, I>::Iterator* it /*not_null*/, int around /*0,1,2*/) const
{
    it->current = this;
    it->around = around;
    return this;
}

template<typename T, typename I>
inline const typename IDelaBella2<T, I>::Simplex*
IDelaBella2<T, I>::Vertex::StartIterator(IDelaBella2<T, I>::Iterator* it /*not_null*/) const
{
    it->current = sew;
    if (sew->v[0] == this)
        it->around = 0;
    else if (sew->v[1] == this)
        it->around = 1;
    else
        it->around = 2;
    return sew;
}
