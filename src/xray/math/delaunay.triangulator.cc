#include "xray/math/delaunay.triangulator.hpp"

template<typename T, typename I>
I
IDelaBella2<T, I>::Prepare(I* start, Simplex** hull, I* out_hull_faces, Simplex** cache, uint64_t* sort_stamp, I stop)
{
    // uint64_t time0 = uSec();

    // if (errlog_proc)
    //	errlog_proc(errlog_file, "[...] sorting vertices");

    I points = inp_verts;

    if (stop >= 0 && points > stop)
        points = stop;

#if 0
		struct CMP
		{
			bool operator () (const Vertex& l, const Vertex& r) const
			{
				// reversing paraboloid, somewhat faster without predicate
				T ax = l.x + tx;
				T ay = l.y + ty;
				T bx = r.x + tx;
				T by = r.y + ty;
				T a = ax * ax + ay * ay;
				T b = bx * bx + by * by;
				if (a == b)
				{
					if (l.x > r.x || l.x == r.x && l.y > r.y)
						return true;
					return false;
				}
				return a > b;
			}

			const T tx, ty;
		} cmp = { trans[0], trans[1] };

		std::sort(vert_alloc, vert_alloc + points, cmp);
#endif

    // rmove dups
    {
        vert_map[vert_alloc[0].i] = 0;

        I w = 0, r = 1; // skip initial no-dups block
        while (r < points && !Vertex::overlap(vert_alloc + r, vert_alloc + w)) {
            vert_map[vert_alloc[r].i] = r;
            w++;
            r++;
        }

        I d = w; // dup map
        w++;

        while (r < points) {
            vert_map[vert_alloc[r].i] = d; // add first dup in run
            r++;

            // skip dups
            while (r < points && Vertex::overlap(vert_alloc + r, vert_alloc + r - 1)) {
                vert_map[vert_alloc[r].i] = d; // add next dup in run
                r++;
            }

            // copy next no-dups block (in percent chunks?)
            while (r < points && !Vertex::overlap(vert_alloc + r, vert_alloc + r - 1)) {
                vert_map[vert_alloc[r].i] = w;
                vert_alloc[w++] = vert_alloc[r++];
            }

            d = w - 1;
        }

// MAKE CRACK IN THE CRYSTAL
// - move last point into front
#if 1
        {
            if (w > 3) {
                const I tail = w - 1;
                Vertex tmp = vert_alloc[tail];
                memmove(vert_alloc + 1, vert_alloc, sizeof(Vertex) * tail);
                vert_alloc[0] = tmp;
                for (int i = 0; i < points; i++) {
                    if (vert_map[i] == tail)
                        vert_map[i] = 0;
                    else
                        vert_map[i]++;
                }
            }
        }
#endif

#if 0
			{
				// more cracks
				const int cracks = 1000;
				if (w > cracks)
				{
					Vert crack[cracks];
					for (I c = 0; c < cracks; c++)
					{
						I d = w - 1 - w * c / cracks;
						I b = w - 1 - w * (c + 1) / cracks;

						crack[c] = vert_alloc[d];
						memmove(vert_alloc + b + c + 2, vert_alloc + b + 1, sizeof(Vert)*(d - b - 1));
					}
					memcpy(vert_alloc, crack, sizeof(Vert)*cracks);

					const I ofs = cracks * (w - 1) / w;
					for (int i = 0; i < points; i++)
					{
						I m = vert_map[i];
						I c  = ofs - cracks * m / w;
						I cc = ofs - cracks * (m + 1) / w;

						if (cc < c)
							vert_map[i] = c;
						else
							vert_map[i] += c + 1;
					}
					
				}
			}
#endif

        uint64_t time1 = uSec();

        // sorting_bench = time1 - *sort_stamp;

        // if (errlog_proc)
        // errlog_proc(errlog_file, "\r[100] sorting vertices (%lld ms)\n", (time1 - *sort_stamp) / 1000);

        *sort_stamp = time1;

        if (points - w) {
            // if (errlog_proc)
            // errlog_proc(errlog_file, "[WRN] detected %d duplicates in xy array!\n", points - w);
            points = w;
        }
    }

    if (points < 3) {
        if (points == 2) {
            // if (errlog_proc)
            // errlog_proc(errlog_file, "[WRN] all input points are colinear, returning single segment!\n");
            first_boundary_vert = vert_alloc + 0;
            first_internal_vert = 0;
            vert_alloc[0].next = vert_alloc + 1;
            vert_alloc[1].next = 0;
        } else {
            // if (errlog_proc)
            // errlog_proc(errlog_file, "[WRN] all input points are identical, returning signle point!\n");
            first_boundary_vert = vert_alloc + 0;
            first_internal_vert = 0;
            vert_alloc[0].next = 0;
        }

        out_boundary_verts = points;
        return -points;
    }

    I i;
    Simplex f; // tmp
    f.v[0] = vert_alloc + 0;

    T lo_x = vert_alloc[0].x, hi_x = lo_x;
    T lo_y = vert_alloc[0].y, hi_y = lo_y;
    I lower_left = 0;
    I upper_right = 0;
    for (i = 1; i < 3; i++) {
        Vertex* v = vert_alloc + i;
        f.v[i] = v;

        if (v->x < lo_x) {
            lo_x = v->x;
            lower_left = i;
        } else if (v->x == lo_x && v->y < vert_alloc[lower_left].y)
            lower_left = i;

        if (v->x > hi_x) {
            hi_x = v->x;
            upper_right = i;
        } else if (v->x == hi_x && v->y > vert_alloc[upper_right].y)
            upper_right = i;

        lo_y = v->y < lo_y ? v->y : lo_y;
        hi_y = v->y > hi_y ? v->y : hi_y;
    }

    int pro = 0;
    // skip until points are coplanar
    while (i < points && f.dot0(vert_alloc[i])) {
        if (i >= pro) {
            uint64_t p = (int)((uint64_t)100 * i / points);
            pro = (int)((p + 1) * points / 100);
            if (pro >= points)
                pro = (int)points - 1;
            if (i == points - 1) {
                p = 100;
            }
            // if (errlog_proc)
            // errlog_proc(errlog_file, "\r[%2d%s] convex hull triangulation ", p, p >= 100 ? "" : "%");
        }

        Vertex* v = vert_alloc + i;

        if (v->x < lo_x) {
            lo_x = v->x;
            lower_left = i;
        } else if (v->x == lo_x && v->y < vert_alloc[lower_left].y)
            lower_left = i;

        if (v->x > hi_x) {
            hi_x = v->x;
            upper_right = i;
        } else if (v->x == hi_x && v->y > vert_alloc[upper_right].y)
            upper_right = i;

        lo_y = v->y < lo_y ? v->y : lo_y;
        hi_y = v->y > hi_y ? v->y : hi_y;

        i++;
    }

    I* vert_sub = 0;

    bool colinear = f.sign0(); // hybrid
    if (colinear) {
        vert_sub = (I*)malloc(sizeof(I) * ((size_t)i + 1));
        if (!vert_sub) {
            // if (errlog_proc)
            // errlog_proc(errlog_file, "[ERR] Not enough memory, shop for some more RAM. See you!\n");
            return 0;
        }

        for (I s = 0; s <= i; s++)
            vert_sub[s] = s;

        // choose x or y axis to sort verts (no need to be exact)
        if (hi_x - lo_x > hi_y - lo_y) {
            struct
            {
                bool operator()(const I& a, const I& b) { return less(vert_alloc[a], vert_alloc[b]); }

                bool less(const Vertex& a, const Vertex& b) const { return a.x < b.x; }

                Vertex* vert_alloc;
            } c;

            c.vert_alloc = vert_alloc;
            std::sort(vert_sub, vert_sub + i, c);
        } else {
            struct
            {
                bool operator()(const I& a, const I& b) { return less(vert_alloc[a], vert_alloc[b]); }

                bool less(const Vertex& a, const Vertex& b) const { return a.y < b.y; }

                Vertex* vert_alloc;
            } c;

            c.vert_alloc = vert_alloc;
            std::sort(vert_sub, vert_sub + i, c);
        }
    } else {
        // split to lower (below diag) and uppert (above diag) parts,
        // sort parts separately in opposite directions
        // mark part with Vertex::sew temporarily

        for (I j = 0; j < i; j++) {
            if (j == lower_left) {
                // lower
                vert_alloc[j].sew = &f;
            } else if (j == upper_right) {
                // upper
                vert_alloc[j].sew = 0;
            } else {
                Vertex* ll = vert_alloc + lower_left;
                Vertex* ur = vert_alloc + upper_right;

                T dot = predicates::adaptive::orient2d(ll->x, ll->y, ur->x, ur->y, vert_alloc[j].x, vert_alloc[j].y);

                if (dot < 0) {
                    // lower
                    vert_alloc[j].sew = &f;
                } else {
                    if (dot == 0) {
                        int dbg = 1;
                    }
#ifdef DELABELLA_AUTOTEST
                    assert(dot > 0);
#endif
                    // upper
                    vert_alloc[j].sew = 0;
                }
            }
        }

        struct
        {
            // default to CW order (unlikely, if wrong, we will reverse using one=2 and two=1)

            bool operator()(const I& a, const I& b) const { return less(vert_alloc[a], vert_alloc[b]); }

            bool less(const Vertex& a, const Vertex& b) const
            {
                // if a is lower and b is upper, return true
                if (a.sew && !b.sew)
                    return false;

                // if a is upper and b is lower, return false
                if (!a.sew && b.sew)
                    return true;

                // actually we can compare coords directly
                if (a.sew) {
                    // lower
                    if (a.x > b.x)
                        return true;
                    if (a.x == b.x)
                        return a.y > b.y;
                    return false;
                } else {
                    // upper
                    if (a.x < b.x)
                        return true;
                    if (a.x == b.x)
                        return a.y < b.y;
                    return false;
                }

// otherwise
#ifdef DELABELLA_AUTOTEST
                assert(0);
#endif
                return false;
            }

            Vertex* vert_alloc;
        } c;

        vert_sub = (I*)malloc(sizeof(I) * ((size_t)i + 1));
        if (!vert_sub) {
            // if (errlog_proc)
            // errlog_proc(errlog_file, "[ERR] Not enough memory, shop for some more RAM. See you!\n");
            return 0;
        }

        for (I s = 0; s <= i; s++)
            vert_sub[s] = s;

        c.vert_alloc = vert_alloc;
        std::sort(vert_sub, vert_sub + i, c);
    }

    *out_hull_faces = 0;

    // alloc faces only if we're going to create them
    if (i < points || !colinear) {
        I hull_faces = 2 * points - 4;
        *out_hull_faces = hull_faces;

        if (max_faces < hull_faces) {
            if (max_faces) {
                free(face_alloc);
                // delete[] face_alloc;
            }
            max_faces = 0;

            /*
            try
            {
                face_alloc = new Simplex[(size_t)hull_faces];
            }
            catch (...)
            {
                face_alloc = 0;
            }
            */

            face_alloc = (Simplex*)malloc(sizeof(Simplex) * hull_faces);

            if (face_alloc)
                max_faces = hull_faces;
            else {
                // if (errlog_proc)
                // errlog_proc(errlog_file, "[ERR] Not enough memory, shop for some more RAM. See you!\n");
                return 0;
            }
        }

        face_alloc[0].flags = 0;
        for (I i = 1; i < hull_faces; i++) {
            face_alloc[i - 1].next = face_alloc + i;
            face_alloc[i].flags = 0;
        }
        face_alloc[hull_faces - 1].next = 0;

        *cache = face_alloc;
    }

    if (i == points) {
        // if (errlog_proc) {
        //     // TODO:
        //     // this should be moved to end of Triangulate()
        //     // after printing final "[100] ... \n"
        //     if (colinear)
        //         errlog_proc(errlog_file, "[WRN] all input points are colinear\n");
        //     else
        //         errlog_proc(errlog_file, "[WRN] all input points are cocircular\n");
        // }

        if (colinear) {
            // link verts into open list
            first_boundary_vert = vert_alloc + vert_sub[0];
            first_internal_vert = 0;
            out_boundary_verts = points;

            for (I j = 1; j < points; j++)
                vert_alloc[j - 1].next = vert_alloc + vert_sub[j];
            vert_alloc[points - 1].next = 0;

            free(vert_sub);
            return -points;
        }

        // we're almost done!
        // time to build final flat hull
        Simplex* next_p = Simplex::Alloc(cache);
        Simplex* next_q = Simplex::Alloc(cache);
        Simplex* prev_p = 0;
        Simplex* prev_q = 0;

        for (I j = 2; j < i; j++) {
            Simplex* p = next_p;
            p->v[0] = vert_alloc + vert_sub[0];
            p->v[1] = vert_alloc + vert_sub[j - 1];
            p->v[2] = vert_alloc + vert_sub[j];

            // mirrored
            Simplex* q = next_q;
            q->v[0] = vert_alloc + vert_sub[0];
            q->v[1] = vert_alloc + vert_sub[j];
            q->v[2] = vert_alloc + vert_sub[j - 1];

            if (j < i - 1) {
                next_p = Simplex::Alloc(cache);
                next_q = Simplex::Alloc(cache);
            } else {
                next_p = 0;
                next_q = 0;
            }

            p->f[0] = q;
            p->f[1] = next_p ? next_p : q;
            p->f[2] = prev_p ? prev_p : q;

            q->f[0] = p;
            q->f[1] = prev_q ? prev_q : p;
            q->f[2] = next_q ? next_q : p;

            prev_p = p;
            prev_q = q;
        }

        *hull = prev_q;
    } else {
        // time to build cone hull with i'th vertex at the tip and 0..i-1 verts in the base
        // build cone's base in direction it is invisible to cone's tip!

        f.v[0] = vert_alloc + vert_sub[0];
        f.v[1] = vert_alloc + vert_sub[1];
        f.v[2] = vert_alloc + vert_sub[2];

        int one = 1, two = 2;
        if (!f.dotNP(vert_alloc[vert_sub[i]])) {
            // if i-th vert can see the contour we will flip every face
            one = 2;
            two = 1;
        }

        Simplex* next_p = Simplex::Alloc(cache);
        Simplex* prev_p = 0;

        Simplex* first_q = 0;
        Simplex* next_q = Simplex::Alloc(cache);
        Simplex* prev_q = 0;

        for (I j = 2; j < i; j++) {
            Simplex* p = next_p;
            p->v[0] = vert_alloc + vert_sub[0];
            p->v[one] = vert_alloc + vert_sub[j - 1];
            p->v[two] = vert_alloc + vert_sub[j];

            Simplex* q;

            if (j == 2) {
                // first base triangle also build extra tip face
                q = Simplex::Alloc(cache);
                q->v[0] = vert_alloc + vert_sub[i];
                q->v[one] = vert_alloc + vert_sub[1];
                q->v[two] = vert_alloc + vert_sub[0];

                q->f[0] = p;
                q->f[one] = 0; // LAST_Q;
                q->f[two] = next_q;

                first_q = q;
                prev_q = q;
            }

            q = next_q;
            q->v[0] = vert_alloc + vert_sub[i];
            q->v[one] = vert_alloc + vert_sub[j];
            q->v[two] = vert_alloc + vert_sub[j - 1];

            next_q = Simplex::Alloc(cache);

            q->f[0] = p;
            q->f[one] = prev_q;
            q->f[two] = next_q;
            prev_q = q;

            p->f[0] = q;

            if (j < i - 1) {
                next_p = Simplex::Alloc(cache);
            } else {
                // last base triangle also build extra tip face
                q = next_q;
                q->v[0] = vert_alloc + vert_sub[i];
                q->v[one] = vert_alloc + vert_sub[0];
                q->v[two] = vert_alloc + vert_sub[i - 1];

                q->f[0] = p;
                q->f[one] = prev_q;
                q->f[two] = first_q;

                first_q->f[one] = q;

                next_p = 0;
                prev_q = q;
            }

            p->f[one] = next_p ? next_p : q;
            p->f[two] = prev_p ? prev_p : first_q;

            prev_p = p;
        }

        *hull = prev_q;

        i++;
    }

    free(vert_sub);

    *start = i;
    return points;
}

template<typename T, typename I>
I
IDelaBella2<T, I>::Triangulate(I* other_faces, uint64_t* sort_stamp, I stop)
{
    I i = 0;
    Simplex* hull = 0;
    I hull_faces = 0;
    Simplex* cache = 0;

    I points = Prepare(&i, &hull, &hull_faces, &cache, sort_stamp, stop);
    unique_points = points < 0 ? -points : points;
    if (points <= 0) {
        return points;
    }

    /////////////////////////////////////////////////////////////////////////
    // ACTUAL ALGORITHM

    /*
    // perf meters
    int seeds = 0;
    int grows = 0;
    */

    int pro = 0;
    for (; i < points; i++) {
        if (i >= pro) {
            uint64_t p = (int)((uint64_t)100 * i / points);
            pro = (int)((p + 1) * points / 100);
            if (pro >= points)
                pro = (int)points - 1;
            if (i == points - 1)
                p = 100;
            if (errlog_proc)
                errlog_proc(errlog_file, "\r[%2d%s] convex hull triangulation ", p, p >= 100 ? "" : "%");
        }

        Vertex* q = vert_alloc + i;
        Vertex* p = vert_alloc + i - 1;
        Simplex* f = hull;

        // 1. FIND FIRST VISIBLE FACE
        //    simply iterate around last vertex using last added triange adjecency info
        while (/*++seeds &&*/ f->dotNP(*q)) {
            f = f->Next(p);
            if (f == hull) {
                // printf(".");
                //  if no visible face can be located at last vertex,
                //  let's run through all faces (approximately last to first),
                //  yes this is emergency fallback and should not ever happen.
                f = face_alloc + (intptr_t)2 * i - 4 - 1;
                while (/*++seeds &&*/ f->dotNP(*q)) {
#ifdef DELABELLA_AUTOTEST

                    if (f == face_alloc) {
                        // dups test
                        struct
                        {
                            bool operator()(const Vertex& a, const Vertex& b) const
                            {
                                return a.x == b.x ? a.y < b.y : a.x < b.x;
                            }
                        } dup;

                        std::sort(vert_alloc, vert_alloc + points, dup);
                        for (I d = 1; d < points; d++) {
                            assert(vert_alloc[d - 1].x != vert_alloc[d].x || vert_alloc[d - 1].y != vert_alloc[d].y);
                        }
                    }

                    assert(f != face_alloc); // no face is visible? you must be kidding!
#endif
                    f--;
                }
            }
        }

        // 2. DELETE VISIBLE FACES & ADD NEW ONES
        //    (we also build silhouette (vertex loop) between visible & invisible faces)

        I del = 0;
        I add = 0;

        // push first visible face onto stack (of visible faces)
        Simplex* stack = f;
        f->next = f; // old trick to use list pointers as 'on-stack' markers
        while (stack) {
            // pop, take care of last item ptr (it's not null!)
            f = stack;
            stack = (Simplex*)f->next;
            if (stack == f)
                stack = 0;
            f->next = 0;

            // copy parts of old face that we still need after removal
            Vertex* fv[3] = { (Vertex*)f->v[0], (Vertex*)f->v[1], (Vertex*)f->v[2] };
            Simplex* ff[3] = { (Simplex*)f->f[0], (Simplex*)f->f[1], (Simplex*)f->f[2] };

            // delete visible face
            f->Free(&cache);
            del++;

            // check all 3 neighbors
            for (int e = 0; e < 3; e++) {
                Simplex* n = ff[e];
                if (n && !n->next) // ensure neighbor is not processed yet & isn't on stack
                {
                    // if neighbor is not visible we have slihouette edge
                    if (/*++grows &&*/ n->dotNP(*q)) {
                        // build face
                        add++;

                        // ab: given face adjacency [index][],
                        // it provides [][2] vertex indices on shared edge (CCW order)
                        const static int ab[3][2] = { { 1, 2 }, { 2, 0 }, { 0, 1 } };

                        Vertex* a = fv[ab[e][0]];
                        Vertex* b = fv[ab[e][1]];

                        Simplex* s = Simplex::Alloc(&cache);
                        s->v[0] = a;
                        s->v[1] = b;
                        s->v[2] = q;

                        s->f[2] = n;

                        // change neighbour's adjacency from old visible face to cone side
                        if (n->f[0] == f)
                            n->f[0] = s;
                        else if (n->f[1] == f)
                            n->f[1] = s;
                        else if (n->f[2] == f)
                            n->f[2] = s;
#ifdef DELABELLA_AUTOTEST
                        else
                            assert(0);
#endif

                        // build silhouette needed for sewing sides in the second pass
                        a->sew = s;
                        a->next = b;
                    } else {
                        // disjoin visible faces
                        // so they won't be processed more than once

                        if (n->f[0] == f)
                            n->f[0] = 0;
                        else if (n->f[1] == f)
                            n->f[1] = 0;
                        else if (n->f[2] == f)
                            n->f[2] = 0;
#ifdef DELABELLA_AUTOTEST
                        else
                            assert(0);
#endif

                        // push neighbor face, it's visible and requires processing
                        n->next = stack ? stack : n;
                        stack = n;
                    }
                }
            }
        }

#ifdef DELABELLA_AUTOTEST
        // if add<del+2 hungry hull has consumed some point
        // that means we can't do delaunay for some under precission reasons
        // althought convex hull would be fine with it
        assert(add == del + 2);
#endif

        // 3. SEW SIDES OF CONE BUILT ON SLIHOUTTE SEGMENTS

        hull = face_alloc + (intptr_t)2 * i - 4 + 1; // last added face

        // last face must contain part of the silhouette
        // (edge between its v[0] and v[1])
        Vertex* entry = (Vertex*)hull->v[0];

        Vertex* pr = entry;
        do {
            // sew pr<->nx
            Vertex* nx = (Vertex*)pr->next;
            pr->sew->f[0] = nx->sew;
            nx->sew->f[1] = pr->sew;
            pr = nx;
        } while (pr != entry);
    }

    // printf("seeds: %d, grows: %d\n", seeds, grows);

#ifdef DELABELLA_AUTOTEST
    assert(2 * i - 4 == hull_faces);
#endif

    for (I j = 0; j < points; j++) {
        vert_alloc[j].next = 0;
        vert_alloc[j].sew = 0;
    }

    I others = 0;

    i = 0;
    Simplex** prev_dela = &first_dela_face;
    Simplex** prev_hull = &first_hull_face;
    for (I j = 0; j < hull_faces; j++) {
        Simplex* f = face_alloc + j;

        // back-link all verts to some_face
        // yea, ~6x times, sorry
        ((Vertex*)f->v[0])->sew = f;
        ((Vertex*)f->v[1])->sew = f;
        ((Vertex*)f->v[2])->sew = f;

        if (f->signN()) {
            f->index = i;          // store index in dela list
            f->flags = 0b01000000; // interior
            *prev_dela = f;
            prev_dela = (Simplex**)&f->next;
            i++;
        } else {
            f->index = others;     // store index in hull list (~ to mark it is not dela)
            f->flags = 0b10000000; // hull
            *prev_hull = f;
            prev_hull = (Simplex**)&f->next;
            others++;
        }
    }

    if (other_faces)
        *other_faces = others;

    *prev_dela = 0;
    *prev_hull = 0;

    // let's trace boudary contour, at least one vertex of first_hull_face
    // must be shared with dela face, find that dela face
    Iter it;
    Vertex* v = (Vertex*)first_hull_face->v[0];
    Simplex* t = (Simplex*)v->StartIterator(&it);
    Simplex* e = t; // end

    first_boundary_vert = (Vertex*)v;
    out_boundary_verts = 1;

    while (1) {
        if (t->IsDelaunay()) {
            int pr = it.around - 1;
            if (pr < 0)
                pr = 2;
            int nx = it.around + 1;
            if (nx > 2)
                nx = 0;

            Simplex* fpr = (Simplex*)t->f[pr];
            if (!fpr->IsDelaunay()) {
                // let's move from: v to t->v[nx]
                v->next = t->v[nx];
                v = (Vertex*)v->next;
                if (v == first_boundary_vert)
                    break; // lap finished
                out_boundary_verts++;
                t = (Simplex*)t->StartIterator(&it, nx);
                e = t;
                continue;
            }
        }
        t = (Simplex*)it.Next();

#ifdef DELABELLA_AUTOTEST
        assert(t != e);
#endif
    }

    // link all other verts into internal list
    first_internal_vert = 0;
    Vertex** prev_inter = &first_internal_vert;
    for (I j = 0; j < points; j++) {
        if (!vert_alloc[j].next) {
            Vertex* next = vert_alloc + j;
            *prev_inter = next;
            prev_inter = (Vertex**)&next->next;
        }
    }

    if (errlog_proc)
        errlog_proc(errlog_file, "\r[100] convex hull triangulation (%lld ms)\n", (uSec() - *sort_stamp) / 1000);

    return 3 * i;
}

template<typename T, typename I>
bool
IDelaBella2<T, I>::ReallocVerts(I points)
{
    inp_verts = points;
    out_verts = 0;
    polygons = 0;

    first_dela_face = 0;
    first_hull_face = 0;
    first_boundary_vert = 0;

    if (max_verts < points) {
        if (max_verts) {
            free(vert_map);
            vert_map = 0;

            free(vert_alloc);
            // delete [] vert_alloc;
            vert_alloc = 0;
            max_verts = 0;
        }

        /*
        try
        {
            vert_alloc = new Vertex[(size_t)points];
        }
        catch (...)
        {
            vert_alloc = 0;
        }
        */

        vert_alloc = (Vertex*)malloc(sizeof(Vertex) * points);

        if (vert_alloc)
            vert_map = (I*)malloc(sizeof(I) * (size_t)points);

        if (vert_alloc && vert_map)
            max_verts = points;
        else {
            if (errlog_proc)
                errlog_proc(errlog_file, "[ERR] Not enough memory, shop for some more RAM. See you!\n");
            return false;
        }
    }

    return true;
}

// private
template<typename T, typename I>
IDellaBella2<T, I>::Simplex*
IDellaBella2<T, I>::FindConstraintOffenders(Vertex* va, Vertex* vb, Simplex*** ptail, Vertex** restart)
{
    static const int rotate[3][3] = { { 0, 1, 2 }, { 1, 2, 0 }, { 2, 0, 1 } };
    static const int other_vert[3][2] = { { 1, 2 }, { 2, 0 }, { 0, 1 } };

    // returns list of faces!
    // with Simplex::index replaced with vertex indice opposite to the offending edge
    // (we are about to change triangulation after all so indexes will change as well)
    // and it's safe for detrimination of dela/hull (sign is preserved)

    Simplex* list = 0;
    Simplex** tail = &list;

    Iter it;

    Simplex* first = (Simplex*)va->StartIterator(&it);
    Simplex* face = first;

    // find first face around va, containing offending edge
    Vertex* v0;
    Vertex* v1;
    Simplex* N = 0;
    int a, b, c;

    while (1) {
        if (!face->IsDelaunay()) {
            face = (Simplex*)it.Next();
#ifdef DELABELLA_AUTOTEST
            assert(face != first);
#endif
            continue;
        }

        a = it.around;
        b = other_vert[a][0];
        v0 = (Vertex*)(face->v[b]);
        if (v0 == vb) {
            *tail = 0;
            *ptail = list ? tail : 0;
            return list; // ab is already there
        }

        c = other_vert[a][1];
        v1 = (Vertex*)(face->v[c]);
        if (v1 == vb) {
            *tail = 0;
            *ptail = list ? tail : 0;
            return list; // ab is already there
        }

        T a0b = predicates::adaptive::orient2d(va->x, va->y, v0->x, v0->y, vb->x, vb->y);
        T a1b = predicates::adaptive::orient2d(va->x, va->y, v1->x, v1->y, vb->x, vb->y);

        if (a0b <= 0 && a1b >= 0) {
            // note:
            // check co-linearity only if v0,v1 are pointing
            // to the right direction (from va to vb)

            if (a0b == 0) {
                *restart = v0;
                *tail = 0;
                *ptail = list ? tail : 0;
                return list;
            }

            if (a1b == 0) {
                *restart = v1;
                *tail = 0;
                *ptail = list ? tail : 0;
                return list;
            }

            // offending edge!
            N = (Simplex*)face;
            break;
        }

        face = (Simplex*)it.Next();

#ifdef DELABELLA_AUTOTEST
        assert(face != first);
#endif
    }

    while (1) {
        if (a) {
            // rotate N->v[] and N->f 'a' times 'backward' such offending edge appears opposite to v[0]
            const int* r = rotate[a];

            Vertex* v[3] = { (Vertex*)N->v[0], (Vertex*)N->v[1], (Vertex*)N->v[2] };
            N->v[0] = v[r[0]];
            N->v[1] = v[r[1]];
            N->v[2] = v[r[2]];

            Simplex* f[3] = { (Simplex*)N->f[0], (Simplex*)N->f[1], (Simplex*)N->f[2] };
            N->f[0] = f[r[0]];
            N->f[1] = f[r[1]];
            N->f[2] = f[r[2]];

            // if (classify)
            {
                // rotate face_bits!
                if (a == 1)
                    N->RotateEdgeFlagsCW();
                else // a==2
                    N->RotateEdgeFlagsCCW();
            }
        }

        // add edge
        *tail = N;
        tail = (Simplex**)&N->next;

        // what is our next face?
        Simplex* F = (Simplex*)(N->f[0]);
        int d, e, f;

        if (F->f[0] == N) {
            d = 0;
            e = 1;
            f = 2;
        } else if (F->f[1] == N) {
            d = 1;
            e = 2;
            f = 0;
        } else {
            d = 2;
            e = 0;
            f = 1;
        }

        Vertex* vr = (Vertex*)(F->v[d]);

        if (vr == vb) {
            *restart = 0;
            *tail = 0;
            *ptail = list ? tail : 0;
            return list;
        }

        // is vr above or below ab ?
        T abr = predicates::adaptive::orient2d(va->x, va->y, vb->x, vb->y, vr->x, vr->y);

        if (abr == 0) {
            *restart = vr;
            *tail = 0;
            *ptail = list ? tail : 0;
            return list;
        }

        if (abr > 0) {
            // above: de edge (a' = f vert)
            a = f;
            v0 = (Vertex*)F->v[d];
            v1 = (Vertex*)F->v[e];
            N = F;
        } else {
            // below: fd edge (a' = e vert)
            a = e;
            v0 = (Vertex*)F->v[f];
            v1 = (Vertex*)F->v[d];
            N = F;
        }
    }

#ifdef DELABELLA_AUTOTEST
    assert(0);
#endif
    *restart = 0;
    *ptail = 0;
    return 0;
}

// private:
template<typename T, typename I>
bool
IDellaBella2<T, I>::FindEdgeFaces(const Vertex* v1, const Vertex* v2, Simplex* twin[2], int opposed[2])
{
    static const int prev[3] = { 2, 0, 1 };
    static const int next[3] = { 1, 2, 0 };

    Iter it;
    Simplex* f = (Simplex*)v1->StartIterator(&it);
    Simplex* e = f;
    do {
        if (f->v[prev[it.around]] == v2) {
            // at least one of them must be dela, it will appear as twin[0]
            if (f->IsDelaunay()) {
                twin[0] = f;
                opposed[0] = next[it.around];
                twin[1] = (Simplex*)it.Next();
                opposed[1] = prev[it.around];

#ifdef DELABELLA_AUTOTEST
                assert(twin[0]->f[opposed[0]] == twin[1]);
                assert(twin[1]->f[opposed[1]] == twin[0]);
#endif

                return true;
            }

            Simplex* tmp_f = f;
            int tmp_op = next[it.around];
            f = (Simplex*)it.Next();

            if (f->IsDelaunay()) {
                twin[0] = f;
                opposed[0] = prev[it.around];
                twin[1] = tmp_f;
                opposed[1] = tmp_op;

#ifdef DELABELLA_AUTOTEST
                assert(twin[0]->f[opposed[0]] == twin[1]);
                assert(twin[1]->f[opposed[1]] == twin[0]);
#endif

                return true;
            }
        } else
            f = (Simplex*)it.Next();
    } while (f != e);

    return false;
}

template<typename T, typename I>
I
IDellaBella2<T, I>::ConstrainEdges(I edges, const I* pa, const I* pb, size_t advance_bytes)
{
    if (advance_bytes == 0)
        advance_bytes = 2 * sizeof(I);

    int unfixed = 0;

    uint64_t time0 = uSec();

    int pro = 0;
    for (I con = 0; con < edges; con++) {
        if (con >= pro) {
            uint64_t p = (int)((uint64_t)100 * con / edges);
            pro = (int)((p + 1) * edges / 100);
            if (pro >= edges)
                pro = (int)edges - 1;
            if (con == edges - 1)
                p = 100;
            if (errlog_proc)
                errlog_proc(errlog_file, "\r[%2d%s] constraining ", p, p >= 100 ? "" : "%");
        }

        I a = *(const I*)((const char*)pa + (intptr_t)con * advance_bytes);
        I b = *(const I*)((const char*)pb + (intptr_t)con * advance_bytes);

        if (!first_dela_face || a == b)
            continue;

        // current
        Vert* va = (Vert*)GetVertexByIndex(a);
        if (!va)
            continue;

        // destination
        Vert* vc = (Vert*)GetVertexByIndex(b);
        if (!vc)
            continue;

        do {
            // 1. Make list of offenders
            Face** tail = 0;
            Vert* restart = 0;
            Face* list = FindConstraintOffenders(va, vc, &tail, &restart);

            if (!list) {
                if (restart) {
                    // if (classify)
                    {
                        Face* nf[2];
                        int op[2];

#ifdef DELABELLA_AUTOTEST
                        assert(FindEdgeFaces(va, restart, nf, op));
#else
                        FindEdgeFaces(va, restart, nf, op);
#endif

                        nf[0]->ToggleEdgeFixed(op[0]);
                        nf[1]->ToggleEdgeFixed(op[1]);
                    }

                    va = restart;
                    continue;
                }

                // if (classify)
                {
                    Face* nf[2];
                    int op[2];

#ifdef DELABELLA_AUTOTEST
                    assert(FindEdgeFaces(va, vc, nf, op));
#else
                    FindEdgeFaces(va, vc, nf, op);
#endif

                    nf[0]->ToggleEdgeFixed(op[0]);
                    nf[1]->ToggleEdgeFixed(op[1]);
                }
            }

            Face* flipped = 0;

            Vert* vb = restart ? restart : vc;

            // will we relink'em back to dela and replace indexes?

            // 2. Repeatedly until there are no offenders on the list
            // - remove first edge from the list of offenders
            // - if it forms concave quad diagonal, push it back to the list of offenders
            // - otherwise do flip then:
            //   - if flipped diagonal still intersects ab, push it back to the list of offenders
            //   - otherwise push it to the list of flipped edges
            while (list) {
                const int a = 0, b = 1, c = 2;

                Face* N = list;
                list = (Face*)N->next;
                N->next = 0;

                Vert* v0 = (Vert*)(N->v[b]);
                Vert* v1 = (Vert*)(N->v[c]);

                Face* F = (Face*)(N->f[a]);
                int d, e, f;

                if (F->f[0] == N) {
                    d = 0;
                    e = 1;
                    f = 2;
                } else if (F->f[1] == N) {
                    d = 1;
                    e = 2;
                    f = 0;
                } else {
                    d = 2;
                    e = 0;
                    f = 1;
                }

                Vert* v = (Vert*)N->v[0]; // may be not same as global va (if we have had a skip)
                Vert* vr = (Vert*)(F->v[d]);

                // is v,v0,vr,v1 a convex quad?
                T v0r = predicates::adaptive::orient2d(v->x, v->y, v0->x, v0->y, vr->x, vr->y);
                T v1r = predicates::adaptive::orient2d(v->x, v->y, v1->x, v1->y, vr->x, vr->y);

                if (v0r >= 0 || v1r <= 0) {
                    // CONCAVE CUNT!
                    *tail = N;
                    tail = (Face**)&N->next;
                    continue;
                }

#ifdef DELABELLA_AUTOTEST
                assert(v0r < 0 && v1r > 0);
#endif

                // if (classify)
                {
                    if (N->IsEdgeFixed(a)) {
                        unfixed++;
#ifdef DELABELLA_AUTOTEST
                        assert(F->IsEdgeFixed(d));
                    } else {
                        assert(!F->IsEdgeFixed(d));
#endif
                    }
                }

                // it's convex, xa already checked
                // if (l_a0r < r_a0r && l_a1r > r_a1r)
                {
                    if (f == 0) {
                        // clang-format off
							/*           *                             *
								   va*  / \                      va*  / \
									  \/   \                        \/   \
									  /\ O  \                       /\ O  \
								   v /  \    \v0                 v /  \    \v0
									*----\----*---------*	      *----\----*---------*
								   / \a   \ b/f\      q/	     / \'-,c\   a\      q/
								  /   \  N \/   \  Q  /   -->   /   \f '-, N  \  Q  /
								 /  P  \   /\ F  \   /		   /  P  \  F '-, b\   /
								/p      \c/e \   d\ /		  /p      \e   \d'-,\ /
							   *---------*----+----*		 *---------*----\----*
										v1     \     vr		           v1    \    vr
												*vb                           *vb
							*/
                        // clang-format on

                        Face* O = (Face*)N->f[c];
                        Face* P = (Face*)(N->f[b]);
                        Face* Q = (Face*)(F->f[e]);

                        // bad if O==P
                        // int p = P->f[0] == N ? 0 : P->f[1] == N ? 1 : 2;
                        // we have to look for Nac edge
                        int p = P->v[0] == v ? 2 : P->v[1] == v ? 0 : 1;

                        // int q = Q->f[0] == F ? 0 : Q->f[1] == F ? 1 : 2;
                        // look for Fdf edge
                        int q = Q->v[0] == vr ? 2 : Q->v[1] == vr ? 0 : 1;

                        // do flip
                        N->v[a] = v0;
                        N->v[b] = vr;
                        N->v[c] = v;
                        N->f[a] = F;
                        N->f[b] = O;
                        N->f[c] = Q;
                        F->v[f] = v; // from v0
                        F->f[d] = P; // from N
                        F->f[e] = N; // from Q
                        P->f[p] = F; // from N
                        Q->f[q] = N; // from F

                        v0->sew = N;
                        v1->sew = F;

                        // if (classify)
                        {
                            // TRANSFER bits:
                            // N->b -> F->d
                            // N->c -> N->b
                            // F->e -> N->c

                            F->SetEdgeBits(d, N->GetEdgeBits(b));
                            N->SetEdgeBits(b, N->GetEdgeBits(c));
                            N->SetEdgeBits(c, F->GetEdgeBits(e));

                            if (v == va && vr == vb) {
                                // set N->a & F->e
                                N->SetEdgeBits(a, 0b00001001);
                                F->SetEdgeBits(e, 0b00001001);
                            } else {
                                // clear N->a & F->e
                                N->SetEdgeBits(a, 0);
                                F->SetEdgeBits(e, 0);
                            }
                        }
                    } else // e==0, (or d==0 but we don't care about it)
                    {
                        // clang-format off
							/*           *						       *
										/p\						      /p\
									   /   \					     /   \
									  /  P  \					    /  P  \
								   v /       \ v0                v /       \ v0
									*---------*          	      *---------*
								   / \a  N  b/f\         	     / \'-,e    f\
							 va*__/___\_____/___\__*vb --> va*__/___\b '-, F__\__*vb
								 /     \   /  F  \             /     \  N '-, d\
								/   O   \c/e     d\  		  /   O   \a    c'-,\
							   *---------*---------*		 *---------*---------*
									   v1 \       / vr		         v1 \       / vr
										   \  Q  /                       \  Q  /
											\   /						  \   /
											 \q/						   \q/
											  *							    *
							*/
                        // clang-format on

                        Face* O = (Face*)N->f[b];

                        Face* P = (Face*)(N->f[c]);
                        Face* Q = (Face*)(F->f[f]);

                        // int p = P->f[0] == N ? 0 : P->f[1] == N ? 1 : 2;
                        // look for Nba edge
                        int p = P->v[0] == v0 ? 2 : P->v[1] == v0 ? 0 : 1;

                        // int q = Q->f[0] == F ? 0 : Q->f[1] == F ? 1 : 2;
                        //  look for Fed edge
                        int q = Q->v[0] == v1 ? 2 : Q->v[1] == v1 ? 0 : 1;

                        // do flip
                        N->v[a] = v1;
                        N->v[b] = v;
                        N->v[c] = vr;
                        N->f[a] = F;
                        N->f[b] = Q;
                        N->f[c] = O;
                        F->v[e] = v; // from v1
                        F->f[d] = P; // from N
                        F->f[f] = N; // from Q
                        P->f[p] = F; // from N
                        Q->f[q] = N; // from F

                        v0->sew = F;
                        v1->sew = N;

                        // if (classify)
                        {
                            // TRANSFER bits:
                            // N->c -> F->d
                            // N->b -> N->c
                            // F->f -> N->b
                            // F->e -> F->e

                            F->SetEdgeBits(d, N->GetEdgeBits(c));
                            N->SetEdgeBits(c, N->GetEdgeBits(b));
                            N->SetEdgeBits(b, F->GetEdgeBits(f));

                            if (v == va && vr == vb) {
                                // set N->a & F->f
                                N->SetEdgeBits(a, 0b00001001);
                                F->SetEdgeBits(f, 0b00001001);
                            } else {
                                N->SetEdgeBits(a, 0);
                                F->SetEdgeBits(f, 0);
                            }
                        }
                    }

                    // if v and vr are on strongly opposite sides of the edge
                    // push N's edge back to offenders otherwise push to the new edges

                    if (va == v || vr == vb) {

                        // resolved!
                        N->next = flipped;
                        flipped = N;
                    } else {
                        // check if v and vr are on the same side of a--b
                        T abv = predicates::adaptive::orient2d(va->x, va->y, vb->x, vb->y, v->x, v->y);
                        T abr = predicates::adaptive::orient2d(va->x, va->y, vb->x, vb->y, vr->x, vr->y);

                        if (abv >= 0 && abr >= 0 || abv <= 0 && abr <= 0) {
                            // resolved
                            N->next = flipped;
                            flipped = N;
                        } else {
                            // unresolved
                            *tail = N;
                            tail = (Face**)&N->next;
                        }
                    }
                }
            }

            // 3. Repeatedly until no flip occurs
            // for every edge from new edges list,
            // if 2 triangles sharing the edge violates delaunay criterion
            // do diagonal flip

            while (1) {
                bool no_flips = true;
                Face* N = flipped;
                while (N) {
                    if (N->v[1] == va && N->v[2] == vb || N->v[1] == vb && N->v[2] == va) {
                        N = (Face*)N->next;
                        continue;
                    }

                    const int a = 0, b = 1, c = 2;

                    Vert* v0 = (Vert*)(N->v[b]);
                    Vert* v1 = (Vert*)(N->v[c]);

                    Face* F = (Face*)(N->f[a]);
                    int d, e, f;

                    if (F->f[0] == N) {
                        d = 0;
                        e = 1;
                        f = 2;
                    } else if (F->f[1] == N) {
                        d = 1;
                        e = 2;
                        f = 0;
                    } else {
                        d = 2;
                        e = 0;
                        f = 1;
                    }

                    Vert* v = (Vert*)N->v[0];
                    Vert* vr = (Vert*)(F->v[d]);

                    // fixed by calling F->cross()
                    // bool np = N->dotP(*vr);
                    // bool fp = F->dotP(*v);
                    // assert(np && fp || !np && !fp);

                    // can we check if it was flipped last time?
                    // if ((N->index & 0x40000000) == 0)
                    if (N->dotP(*vr) /* || F->dotP(*v)*/) {
                        // if (classify)
                        {
                            // check if N->a or F->d is marked as fixed
                            // it should not ever happen -- i think

                            // ... but if it does happen
                            // remember we've broken fixed edge
                            // then display warning right before we return

                            if (N->IsEdgeFixed(a)) {
                                printf("IT DOES HAPPEN!\n");
                                unfixed++;
#ifdef DELABELLA_AUTOTEST
                                assert(F->IsEdgeFixed(d));
                            } else {
                                assert(!F->IsEdgeFixed(d));
#endif
                            }
                        }

                        no_flips = false;

                        if (f == 0) {
                            Face* O = (Face*)N->f[c];
                            Face* P = (Face*)(N->f[b]);
                            Face* Q = (Face*)(F->f[e]);

                            // bad if O==P
                            // int p = P->f[0] == N ? 0 : P->f[1] == N ? 1 : 2;
                            // we have to look for Nac edge
                            int p = P->v[0] == v ? 2 : P->v[1] == v ? 0 : 1;

                            // int q = Q->f[0] == F ? 0 : Q->f[1] == F ? 1 : 2;
                            // look for Fdf edge
                            int q = Q->v[0] == vr ? 2 : Q->v[1] == vr ? 0 : 1;

                            // do flip
                            N->v[a] = v0;
                            N->v[b] = vr;
                            N->v[c] = v;
                            N->f[a] = F;
                            N->f[b] = O;
                            N->f[c] = Q;
                            F->v[f] = v; // from v0
                            F->f[d] = P; // from N
                            F->f[e] = N; // from Q
                            P->f[p] = F; // from N
                            Q->f[q] = N; // from F

                            v0->sew = N;
                            v1->sew = F;

                            // if (classify)
                            {
                                // TRANSFER bits:
                                // N->b -> F->d
                                // N->c -> N->b
                                // F->f -> F->f
                                // F->e -> N->c

                                F->SetEdgeBits(d, N->GetEdgeBits(b));
                                N->SetEdgeBits(b, N->GetEdgeBits(c));
                                N->SetEdgeBits(c, F->GetEdgeBits(e));

                                // clear N->a & F->e
                                N->SetEdgeBits(a, 0);
                                F->SetEdgeBits(e, 0);
                            }
                        } else {
                            Face* O = (Face*)N->f[b];
                            Face* P = (Face*)(N->f[c]);
                            Face* Q = (Face*)(F->f[f]);

                            // int p = P->f[0] == N ? 0 : P->f[1] == N ? 1 : 2;
                            // look for Nba edge
                            int p = P->v[0] == v0 ? 2 : P->v[1] == v0 ? 0 : 1;

                            // int q = Q->f[0] == F ? 0 : Q->f[1] == F ? 1 : 2;
                            //  look for Fed edge
                            int q = Q->v[0] == v1 ? 2 : Q->v[1] == v1 ? 0 : 1;

                            // do flip
                            N->v[a] = v1;
                            N->v[b] = v;
                            N->v[c] = vr;
                            N->f[a] = F;
                            N->f[b] = Q;
                            N->f[c] = O;
                            F->v[e] = v; // from v1
                            F->f[d] = P; // from N
                            F->f[f] = N; // from Q
                            P->f[p] = F; // from N
                            Q->f[q] = N; // from F

                            v0->sew = F;
                            v1->sew = N;

                            // if (classify)
                            {
                                // TRANSFER bits:
                                // N->c -> F->d
                                // N->b -> N->c
                                // F->f -> N->b
                                // F->e -> F->e

                                F->SetEdgeBits(d, N->GetEdgeBits(c));
                                N->SetEdgeBits(c, N->GetEdgeBits(b));
                                N->SetEdgeBits(b, F->GetEdgeBits(f));

                                // clear N->a & F->f
                                N->SetEdgeBits(a, 0);
                                F->SetEdgeBits(f, 0);
                            }
                        }

                        // can we un-mark not flipped somehow?
                        // N->index &= 0x3fffffff;
                        // F->index &= 0x3fffffff;
                    } else {
                        // can we mark it as not flipped somehow?
                        // N->index |= 0x40000000;
                    }

                    N = (Face*)N->next;
                }

                if (no_flips)
                    break;
            }

            va = restart;
        } while (va);
    }

    // clean up the mess we've made with dela faces list !!!
    I hull_faces = 2 * unique_points - 4;
    Face** tail = &first_dela_face;
    I index = 0;
    for (I i = 0; i < hull_faces; i++) {
        if (face_alloc[i].IsDelaunay()) {
            *tail = face_alloc + i;
            tail = (Face**)&face_alloc[i].next;
            face_alloc[i].index = index++;
        }
    }
    *tail = 0;

    polygons = index;

    if (errlog_proc) {
        errlog_proc(errlog_file, "\r[100] constraining (%lld ms)\n", (uSec() - time0) / 1000);
        if (unfixed)
            errlog_proc(errlog_file, "\r[WRN] %d crossing edges detected - unfixed\n", unfixed);
    }

    return polygons;
}
