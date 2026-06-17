#include <algorithm>
#include <cassert>
#include "BVH.hpp"
#include <cfloat>
#include <sys/time.h>
#include <chrono>
#include <sys/time.h>

BVHAccel::BVHAccel(std::vector<Object*> p, int maxPrimsInNode,
                   SplitMethod splitMethod)
    : maxPrimsInNode(std::min(255, maxPrimsInNode)), splitMethod(splitMethod),
      primitives(std::move(p))
{
    // time_t start, stop;
    // time(&start);
    auto start = std::chrono::system_clock::now();

    if (primitives.empty())
        return;

    // modified by skc begin
    if (splitMethod == BVHAccel::SplitMethod::NAIVE) {
        std::cout << "generate BVH by naive method" << std::endl;
        root = recursiveBuild(primitives);
    } else if (splitMethod == BVHAccel::SplitMethod::SAH) {
        std::cout << "generate BVH by SAH method" << std::endl;
        root = recursiveBuildBySAH(primitives);
    }
    // modified by skc end

    auto stop = std::chrono::system_clock::now();

    std::cout << "\nBVH Generation complete, total time: ";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " ms\n";

}

BVHBuildNode* BVHAccel::recursiveBuild(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (int i = 0; i < objects.size(); ++i)
        bounds = Union(bounds, objects[i]->getBounds());
    if (objects.size() == 1) {
        // Create leaf _BVHBuildNode_
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuild(std::vector{objects[0]});
        node->right = recursiveBuild(std::vector{objects[1]});

        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }
    else {
        Bounds3 centroidBounds;
        for (int i = 0; i < objects.size(); ++i)
            centroidBounds =
                Union(centroidBounds, objects[i]->getBounds().Centroid());
        int dim = centroidBounds.maxExtent();
        switch (dim) {
        case 0:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().x <
                       f2->getBounds().Centroid().x;
            });
            break;
        case 1:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().y <
                       f2->getBounds().Centroid().y;
            });
            break;
        case 2:
            std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().z <
                       f2->getBounds().Centroid().z;
            });
            break;
        }

        auto beginning = objects.begin();
        auto middling = objects.begin() + (objects.size() / 2);
        auto ending = objects.end();

        auto leftshapes = std::vector<Object*>(beginning, middling);
        auto rightshapes = std::vector<Object*>(middling, ending);

        assert(objects.size() == (leftshapes.size() + rightshapes.size()));

        node->left = recursiveBuild(leftshapes);
        node->right = recursiveBuild(rightshapes);

        node->bounds = Union(node->left->bounds, node->right->bounds);
    }

    return node;
}

// skc add start
BVHBuildNode* BVHAccel::recursiveBuildBySAH(std::vector<Object*> objects)
{
    BVHBuildNode* node = new BVHBuildNode();

    // Compute bounds of all primitives in BVH node
    Bounds3 bounds;
    for (int i = 0; i < objects.size(); ++i)
        bounds = Union(bounds, objects[i]->getBounds());
    if (objects.size() == 1) {
        // Create leaf _BVHBuildNode_
        node->bounds = objects[0]->getBounds();
        node->object = objects[0];
        node->left = nullptr;
        node->right = nullptr;
        return node;
    }
    else if (objects.size() == 2) {
        node->left = recursiveBuildBySAH(std::vector{objects[0]});
        node->right = recursiveBuildBySAH(std::vector{objects[1]});

        node->bounds = Union(node->left->bounds, node->right->bounds);
        return node;
    }
    else {
        Bounds3 centroidBounds;
        for (int i = 0; i < objects.size(); ++i)
            centroidBounds =
                Union(centroidBounds, objects[i]->getBounds().Centroid());
        int dim = centroidBounds.maxExtent();

        std::vector<Object*> leftshapes;
        std::vector<Object*> rightshapes;
        int buckets_num = 12;
        float suraceArea = centroidBounds.SurfaceArea();

        switch (dim) {
        case 0: {
            // x as longest axis
            Object* start = objects[0];
            Object* last = objects[0];
            float x_min = FLT_MAX;
            float x_max = FLT_MIN;

            for (int i = 0; i < objects.size(); i++) {
                if (objects[i]->getBounds().Centroid().x < x_min) {
                    x_min = objects[i]->getBounds().Centroid().x;
                    start = objects[i];
                }
                if (objects[i]->getBounds().Centroid().x > x_max) {
                    x_max = objects[i]->getBounds().Centroid().x;
                    last = objects[i];
                }
            }
            // SVH core strategy begin
          
            float length = last->getBounds().Centroid().x - start->getBounds().Centroid().x;
            float best_svh_cost = FLT_MAX;
            int best_split_position = -1;

            for (int j = 1; j <= buckets_num-1; j++) {
                float split_position =  start->getBounds().Centroid().x + j*length/buckets_num;
                Vector3f new_pMax = centroidBounds.pMax;
                new_pMax.x = split_position;
                Vector3f new_pMin = centroidBounds.pMin;
                new_pMin.x = split_position;
                Bounds3 left_bounds(centroidBounds.pMin, new_pMax);
                Bounds3 right_bounds(new_pMin, centroidBounds.pMax);
                Vector3f left_diagonal = left_bounds.Diagonal();
                Vector3f right_diagonal = right_bounds.Diagonal();
                float left_surfaceArea = 2*(left_diagonal.x*left_diagonal.y + left_diagonal.x*left_diagonal.z + left_diagonal.y*left_diagonal.z);
                float right_surfaceArea = 2*(right_diagonal.x*right_diagonal.y + right_diagonal.x*right_diagonal.z + right_diagonal.y*right_diagonal.z);
                int left_num = 0;
                int right_num = 0;
                for (int i = 0; i < objects.size(); i++) {
                    if (objects[i]->getBounds().Centroid().x < split_position) {
                        left_num++;
                    } else if (objects[i]->getBounds().Centroid().x >= split_position) {
                        right_num++;
                    }
                }
                float svh_cost = 0.125f+left_num*left_surfaceArea/suraceArea + right_num*right_surfaceArea/suraceArea;
                if (svh_cost < best_svh_cost) {
                    best_svh_cost = svh_cost;
                    best_split_position = split_position;
                }
            }

            for (int i = 0; i < objects.size(); i++) {
                if (objects[i]->getBounds().Centroid().x < best_split_position) {
                    leftshapes.emplace_back(objects[i]);
                } else if (objects[i]->getBounds().Centroid().x >= best_split_position) {
                    rightshapes.emplace_back(objects[i]);
                }
            }
            // SVH end
            break;
        }
        case 1: {
            // y as longest axis
            Object* start = objects[0];
            Object* last = objects[0];
            float y_min = FLT_MAX;
            float y_max = FLT_MIN;

            for (int i = 0; i < objects.size(); i++) {
                if (objects[i]->getBounds().Centroid().y < y_min) {
                    y_min = objects[i]->getBounds().Centroid().y;
                    start = objects[i];
                }
                if (objects[i]->getBounds().Centroid().y > y_max) {
                    y_max = objects[i]->getBounds().Centroid().y;
                    last = objects[i];
                }
            }
            // SVH core strategy begin
  
            float length = last->getBounds().Centroid().y - start->getBounds().Centroid().y;
            float best_svh_cost = FLT_MAX;
            int best_split_position = -1;

            for (int j = 1; j <= buckets_num-1; j++) {
                float split_position =  start->getBounds().Centroid().y + j*length/buckets_num;
                Vector3f new_pMax = centroidBounds.pMax;
                new_pMax.y = split_position;
                Vector3f new_pMin = centroidBounds.pMin;
                new_pMin.y = split_position;
                Bounds3 left_bounds(centroidBounds.pMin, new_pMax);
                Bounds3 right_bounds(new_pMin, centroidBounds.pMax);
                Vector3f left_diagonal = left_bounds.Diagonal();
                Vector3f right_diagonal = right_bounds.Diagonal();
                float left_surfaceArea = 2*(left_diagonal.x*left_diagonal.y + left_diagonal.x*left_diagonal.z + left_diagonal.y*left_diagonal.z);
                float right_surfaceArea = 2*(right_diagonal.x*right_diagonal.y + right_diagonal.x*right_diagonal.z + right_diagonal.y*right_diagonal.z);
                int left_num = 0;
                int right_num = 0;
                for (int i = 0; i < objects.size(); i++) {
                    if (objects[i]->getBounds().Centroid().y < split_position) {
                        left_num++;
                    } else if (objects[i]->getBounds().Centroid().y >= split_position) {
                        right_num++;
                    }
                }
                float svh_cost = 0.125f+left_num*left_surfaceArea/suraceArea + right_num*right_surfaceArea/suraceArea;
                if (svh_cost < best_svh_cost) {
                    best_svh_cost = svh_cost;
                    best_split_position = split_position;
                }
            }

            for (int i = 0; i < objects.size(); i++) {
                if (objects[i]->getBounds().Centroid().y < best_split_position) {
                    leftshapes.emplace_back(objects[i]);
                } else if (objects[i]->getBounds().Centroid().y >= best_split_position) {
                    rightshapes.emplace_back(objects[i]);
                }
            }
            // SVH end

            break;
        }
        case 2: {
            // z as longest axis
            Object* start = objects[0];
            Object* last = objects[0];
            float z_min = FLT_MAX;
            float z_max = FLT_MIN;

            for (int i = 0; i < objects.size(); i++) {
                if (objects[i]->getBounds().Centroid().z < z_min) {
                    z_min = objects[i]->getBounds().Centroid().z;
                    start = objects[i];
                }
                if (objects[i]->getBounds().Centroid().z > z_max) {
                    z_max = objects[i]->getBounds().Centroid().z;
                    last = objects[i];
                }
            }
            
            // SVH core strategy begin

            float length = last->getBounds().Centroid().z - start->getBounds().Centroid().z;
            float best_svh_cost = FLT_MAX;
            int best_split_position = -1;

            for (int j = 1; j <= buckets_num-1; j++) {
                float split_position =  start->getBounds().Centroid().z + j*length/buckets_num;
                Vector3f new_pMax = centroidBounds.pMax;
                new_pMax.z = split_position;
                Vector3f new_pMin = centroidBounds.pMin;
                new_pMin.z = split_position;
                Bounds3 left_bounds(centroidBounds.pMin, new_pMax);
                Bounds3 right_bounds(new_pMin, centroidBounds.pMax);
                Vector3f left_diagonal = left_bounds.Diagonal();
                Vector3f right_diagonal = right_bounds.Diagonal();
                float left_surfaceArea = 2*(left_diagonal.x*left_diagonal.y + left_diagonal.x*left_diagonal.z + left_diagonal.y*left_diagonal.z);
                float right_surfaceArea = 2*(right_diagonal.x*right_diagonal.y + right_diagonal.x*right_diagonal.z + right_diagonal.y*right_diagonal.z);
                int left_num = 0;
                int right_num = 0;
                for (int i = 0; i < objects.size(); i++) {
                    if (objects[i]->getBounds().Centroid().z < split_position) {
                        left_num++;
                    } else if (objects[i]->getBounds().Centroid().z >= split_position) {
                        right_num++;
                    }
                }
                float svh_cost = 0.125f+left_num*left_surfaceArea/suraceArea + right_num*right_surfaceArea/suraceArea;
                if (svh_cost < best_svh_cost) {
                    best_svh_cost = svh_cost;
                    best_split_position = split_position;
                }
            }

            for (int i = 0; i < objects.size(); i++) {
                if (objects[i]->getBounds().Centroid().z < best_split_position) {
                    leftshapes.emplace_back(objects[i]);
                } else if (objects[i]->getBounds().Centroid().z >= best_split_position) {
                    rightshapes.emplace_back(objects[i]);
                }
            }
            // SVH end

            break;
        }
        }

        // in case of empty, causing stack overflow and segmentation fault
        if (leftshapes.empty() || rightshapes.empty())
        {
            leftshapes.clear();
            rightshapes.clear();
            Bounds3 centroidBounds;
            for (int i = 0; i < objects.size(); ++i)
                centroidBounds =
                    Union(centroidBounds, objects[i]->getBounds().Centroid());
            int dim = centroidBounds.maxExtent();
            switch (dim) {
            case 0:
                std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                return f1->getBounds().Centroid().x <
                       f2->getBounds().Centroid().x;
                });
                break;
            case 1:
                std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                    return f1->getBounds().Centroid().y <
                       f2->getBounds().Centroid().y;
                });
                break;
            case 2:
                std::sort(objects.begin(), objects.end(), [](auto f1, auto f2) {
                    return f1->getBounds().Centroid().z <
                       f2->getBounds().Centroid().z;
                });
                break;
            }

            auto beginning = objects.begin();
            auto middling = objects.begin() + (objects.size() / 2);
            auto ending = objects.end();

            leftshapes = std::vector<Object*>(beginning, middling);
            rightshapes = std::vector<Object*>(middling, ending);
        }

        assert(objects.size() == (leftshapes.size() + rightshapes.size()));
        assert(!leftshapes.empty() && !rightshapes.empty());

        node->left = recursiveBuildBySAH(leftshapes);
        node->right = recursiveBuildBySAH(rightshapes);

        node->bounds = Union(node->left->bounds, node->right->bounds);
    }

    return node;
}
// skc add end

// struct BucketInfo {
//     Bounds3 bounds;
//     int count;
// };

// BVHBuildNode* BVHAccel::recursiveBuildBySAH(std::vector<Object*> objects)
// {
//     BVHBuildNode* node = new BVHBuildNode();

//     // Compute bounds of all primitives in BVH node
//     Bounds3 bounds;
//     for (int i = 0; i < objects.size(); ++i)
//         bounds = Union(bounds, objects[i]->getBounds());
//    //假如物体数目过少 就直接使用原来的划分方法 没必要
//     if (objects.size() <= 2) {
//         node = recursiveBuild(objects);
//         return node;
//     }
//     else {
//         Bounds3 centroidBounds;
//         for (int i = 0; i < objects.size(); ++i)
//             centroidBounds =
//             Union(centroidBounds, objects[i]->getBounds().Centroid());
//         int dim = centroidBounds.maxExtent();
//         //计算索引
//         constexpr int nBuckets = 8;
//         BucketInfo buckets[nBuckets];
//         for (int i = 0; i < objects.size(); ++i) {
//             int b = nBuckets *
//                 centroidBounds.Offset(
//                     objects[i]->getBounds().Centroid())[dim];
//             if (b == nBuckets) b = nBuckets - 1;
//             buckets[b].count++;
//             buckets[b].bounds =
//                 Union(buckets[b].bounds, objects[i]->getBounds());
//         }
//         //计算每个划分的代价
//         float cost[nBuckets - 1];
//         for (int i = 0; i < nBuckets - 1; ++i) {
//             Bounds3 b0, b1;
//             int count0 = 0, count1 = 0;
//             for (int j = 0; j <= i; ++j) {
//                 b0 = Union(b0, buckets[j].bounds);
//                 count0 += buckets[j].count;
//             }
//             for (int j = i + 1; j < nBuckets; ++j) {
//                 b1 = Union(b1, buckets[j].bounds);
//                 count1 += buckets[j].count;
//             }
//             cost[i] = 0.125f +
//                 (count0 * b0.SurfaceArea() +
//                     count1 * b1.SurfaceArea()) /
//                 bounds.SurfaceArea();
//         }
//         //找出最小代价
//         float minCost = cost[0];
//         int minCostSplitBucket = 0;
//         for (int i = 1; i < nBuckets - 1; ++i) {
//             if (cost[i] < minCost) {
//                 minCost = cost[i];
//                 minCostSplitBucket = i;
//             }
//         }
//         float leafCost = objects.size();
//         int mid = 0;
//         //如果包含的物体过多 或者缩小划分代价 则进行划分
//         if (objects.size() > maxPrimsInNode || minCost < leafCost) {
//             for (int i = 0; i < objects.size(); ++i) {
//                 int b = nBuckets *
//                     centroidBounds.Offset(
//                         objects[i]->getBounds().Centroid())[dim];
//                 if (b == nBuckets) b = nBuckets - 1;

//                 // 满足条件的元素放到数组前半部分
//                 if (b <= minCostSplitBucket) {
//                     std::swap(objects[i], objects[mid]);
//                     mid++;
//                 }
//             }
//         }
       
//         auto beginning = objects.begin();
//         auto middling = objects.begin() + mid;
//         auto ending = objects.end();

//         auto leftshapes = std::vector<Object*>(beginning, middling);
//         auto rightshapes = std::vector<Object*>(middling, ending);

//         assert(objects.size() == (leftshapes.size() + rightshapes.size()));

//         node->left = recursiveBuildBySAH(leftshapes);
//         node->right = recursiveBuildBySAH(rightshapes);

//         node->bounds = Union(node->left->bounds, node->right->bounds);
//     }

//     return node;
// }



Intersection BVHAccel::Intersect(const Ray& ray) const
{
    Intersection isect;
    if (!root)
        return isect;
    isect = BVHAccel::getIntersection(root, ray);
    return isect;
}

Intersection BVHAccel::getIntersection(BVHBuildNode* node, const Ray& ray) const
{
    // TODO Traverse the BVH to find intersection

    // add by skc begin
    Intersection intersect;
    if (node == nullptr) return intersect;

    std::array<int, 3> dirIsNeg = {int(ray.direction.x>0),int(ray.direction.y>0),int(ray.direction.z>0)};

    if (!node->bounds.IntersectP(ray, ray.direction_inv, dirIsNeg)) {
        return intersect;
    }

    if (node->object != nullptr) {
        // Only one object in a node in this case; no loop
        return node->object->getIntersection(ray);
    }

    Intersection hit1 = getIntersection(node->left, ray);
    Intersection hit2 = getIntersection(node->right, ray);

    if (hit1.happened && hit2.happened) {
        return (hit1.distance < hit2.distance) ? hit1 : hit2;
    } else if (hit1.happened) {
        return hit1;
    } else if (hit2.happened) {
        return hit2;
    }

    return intersect;
    // add by skc end
}