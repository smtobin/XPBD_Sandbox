#include "MeshUtils.hpp"

#include <iostream>
#include <fstream>

void MeshUtils::loadSurfaceMeshFromFile(const std::string& filename, Eigen::Matrix<double, -1, 3>& verts, Eigen::Matrix<unsigned, -1, 3>& faces)
{
    Assimp::Importer importer;

    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll
    // probably to request more postprocessing than we do in this example.
    const aiScene* scene = importer.ReadFile( filename,
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_SortByPType);

    // If the import failed, report it
    if (scene == nullptr)
    {
        std::cerr << "\tAssimp::Importer could not open " << filename << std::endl;
        std::cerr << "\tEnsure that the file is in a format that assimp can handle." << std::endl;
        return;
    }

    const aiMesh* mesh = scene->mMeshes[0];

    // Extract vertices
    verts.conservativeResize(mesh->mNumVertices, 3);
    for (unsigned i = 0; i < mesh->mNumVertices; i++)
    {
        verts(i,0) = mesh->mVertices[i].x;
        verts(i,1) = mesh->mVertices[i].y;
        verts(i,2) = mesh->mVertices[i].z;
    }

    // Extract faces
    faces.conservativeResize(mesh->mNumFaces, 3);
    for (unsigned i = 0; i < mesh->mNumFaces; i++)
    {
        faces(i,0) = mesh->mFaces[i].mIndices[0];
        faces(i,1) = mesh->mFaces[i].mIndices[1];
        faces(i,2) = mesh->mFaces[i].mIndices[2];
    }
}

void MeshUtils::createBeamObjWithOffsetVerts(const std::string& filename, const double length, const double width, const double height)
{
    // try and discretize so that the elements are roughly cube
    const double elem_size = std::min(width, height);
    const int w = static_cast<int>(width/elem_size);
    const int h = static_cast<int>(height/elem_size);
    const int l = static_cast<int>(length/elem_size);
    std::cout << "size in elements: " << h << "x" << w << "x" << l << std::endl;
    
    auto aligned_vert_index = [w, h] (int li, int wi, int hi)
    {
        return hi + wi*(h+1) + li*(w+1)*(h+1);
    };

    // auto offset_vert_index = [l, w, h] (int li, int wi, int hi)
    // {
    //     return (l+1)*(w+1)*(h+1) + hi + wi*h + li*w*h;
    // };

    Eigen::Matrix<double, -1, 3> verts((h+1)*(w+1)*(l+1) + (2*h*w + 2*h*l + 2*w*l), 3);
    for (int li = 0; li < l+1; li++) 
    {
        for (int wi = 0; wi < w+1; wi++)
        {
            for (int hi = 0; hi < h+1; hi++)
            {
                int ind = aligned_vert_index(li, wi, hi);
                Eigen::Vector3d aligned_vert({wi*elem_size, li*elem_size, hi*elem_size});
                verts.row(ind) = aligned_vert;
            }
        }
    }

    Eigen::Matrix<int, -1, 3> faces(8*w*h + 8*l*h + 8*l*w, 3);
    int face_ind = 0;
    int vert_ind = (h+1)*(w+1)*(l+1);
    // w*h faces
    for (int wi = 0; wi < w; wi++)
    {
        for (int hi = 0; hi < h; hi++)
        {
            Eigen::Vector3d offset_vertf({wi*elem_size + elem_size/2.0, 0, hi*elem_size + elem_size/2.0});
            verts.row(vert_ind) = offset_vertf;
            Eigen::Vector3d offset_vertb({wi*elem_size + elem_size/2.0, l*elem_size, hi*elem_size + elem_size/2.0});
            verts.row(vert_ind+1) = offset_vertb;

            int ff1 = aligned_vert_index(0, wi, hi);
            int ff2 = aligned_vert_index(0, wi+1, hi);
            int ff3 = aligned_vert_index(0, wi+1, hi+1);
            int ff4 = aligned_vert_index(0, wi, hi+1);
            int ff5 = vert_ind;//offset_vert_index(0, wi, hi);

            Eigen::Vector3i front_face1({ff1, ff2, ff5});
            Eigen::Vector3i front_face2({ff2, ff3, ff5});
            Eigen::Vector3i front_face3({ff3, ff4, ff5});
            Eigen::Vector3i front_face4({ff4, ff1, ff5});
            faces.row(face_ind) = front_face1;
            faces.row(face_ind+1) = front_face2;
            faces.row(face_ind+2) = front_face3;
            faces.row(face_ind+3) = front_face4;

            int bf1 = aligned_vert_index(l, wi, hi);
            int bf2 = aligned_vert_index(l, wi+1, hi);
            int bf3 = aligned_vert_index(l, wi+1, hi+1);
            int bf4 = aligned_vert_index(l, wi, hi+1);
            int bf5 = vert_ind+1;//offset_vert_index(l, wi, hi);
            Eigen::Vector3i back_face1({bf1, bf2, bf5});
            Eigen::Vector3i back_face2({bf2, bf3, bf5});
            Eigen::Vector3i back_face3({bf3, bf4, bf5});
            Eigen::Vector3i back_face4({bf4, bf1, bf5});
            faces.row(face_ind+4) = back_face1;
            faces.row(face_ind+5) = back_face2;
            faces.row(face_ind+6) = back_face3;
            faces.row(face_ind+7) = back_face4;

            face_ind += 8;
            vert_ind += 2;
        }
    }

    // l*h faces
    for (int hi = 0; hi < h; hi++)
    {
        for (int li = 0; li < l; li++)
        {
            Eigen::Vector3d offset_vertr({w*elem_size, li*elem_size + elem_size/2.0, hi*elem_size + elem_size/2.0});
            verts.row(vert_ind) = offset_vertr;
            Eigen::Vector3d offset_vertl({0, li*elem_size + elem_size/2.0, hi*elem_size + elem_size/2.0});
            verts.row(vert_ind+1) = offset_vertl;

            int rf1 = aligned_vert_index(li, w, hi);
            int rf2 = aligned_vert_index(li+1, w, hi);
            int rf3 = aligned_vert_index(li+1, w, hi+1);
            int rf4 = aligned_vert_index(li, w, hi+1);
            int rf5 = vert_ind;//offset_vert_index(li, w, hi);
            Eigen::Vector3i right_face1({rf1, rf2, rf5});
            Eigen::Vector3i right_face2({rf2, rf3, rf5});
            Eigen::Vector3i right_face3({rf3, rf4, rf5});
            Eigen::Vector3i right_face4({rf4, rf1, rf5});
            faces.row(face_ind) = right_face1;
            faces.row(face_ind+1) = right_face2;
            faces.row(face_ind+2) = right_face3;
            faces.row(face_ind+3) = right_face4;

            int lf1 = aligned_vert_index(li, 0, hi);
            int lf2 = aligned_vert_index(li+1, 0, hi);
            int lf3 = aligned_vert_index(li+1, 0, hi+1);
            int lf4 = aligned_vert_index(li, 0, hi+1);
            int lf5 = vert_ind+1;//offset_vert_index(li, 0, hi);
            Eigen::Vector3i left_face1({lf1, lf2, lf5});
            Eigen::Vector3i left_face2({lf2, lf3, lf5});
            Eigen::Vector3i left_face3({lf3, lf4, lf5});
            Eigen::Vector3i left_face4({lf4, lf1, lf5});
            faces.row(face_ind+4) = left_face1;
            faces.row(face_ind+5) = left_face2;
            faces.row(face_ind+6) = left_face3;
            faces.row(face_ind+7) = left_face4;

            face_ind += 8;
            vert_ind += 2;
        }
    }

    // l*w faces
    for (int li = 0; li < l; li++)
    {
        for (int wi = 0; wi < w; wi++)
        {
            Eigen::Vector3d offset_vertt({wi*elem_size + elem_size/2.0, li*elem_size + elem_size/2.0, h*elem_size});
            verts.row(vert_ind) = offset_vertt;
            Eigen::Vector3d offset_vertb({wi*elem_size + elem_size/2.0, li*elem_size + elem_size/2.0, 0});
            verts.row(vert_ind+1) = offset_vertb;

            int tf1 = aligned_vert_index(li, wi, h);
            int tf2 = aligned_vert_index(li, wi+1, h);
            int tf3 = aligned_vert_index(li+1, wi+1, h);
            int tf4 = aligned_vert_index(li+1, wi, h);
            int tf5 = vert_ind;//offset_vert_index(li, wi, h);
            Eigen::Vector3i top_face1({tf1, tf2, tf5});
            Eigen::Vector3i top_face2({tf2, tf3, tf5});
            Eigen::Vector3i top_face3({tf3, tf4, tf5});
            Eigen::Vector3i top_face4({tf4, tf1, tf5});
            faces.row(face_ind) = top_face1;
            faces.row(face_ind+1) = top_face2;
            faces.row(face_ind+2) = top_face3;
            faces.row(face_ind+3) = top_face4;

            int bf1 = aligned_vert_index(li, wi, 0);
            int bf2 = aligned_vert_index(li, wi+1, 0);
            int bf3 = aligned_vert_index(li+1, wi+1, 0);
            int bf4 = aligned_vert_index(li+1, wi, 0);
            int bf5 = vert_ind+1;//offset_vert_index(li, wi, 0);
            Eigen::Vector3i bottom_face1({bf1, bf2, bf5});
            Eigen::Vector3i bottom_face2({bf2, bf3, bf5});
            Eigen::Vector3i bottom_face3({bf3, bf4, bf5});
            Eigen::Vector3i bottom_face4({bf4, bf1, bf5});
            faces.row(face_ind+4) = bottom_face1;
            faces.row(face_ind+5) = bottom_face2;
            faces.row(face_ind+6) = bottom_face3;
            faces.row(face_ind+7) = bottom_face4;

            face_ind += 8;
            vert_ind += 2;
        }
    }

    std::ofstream ss(filename);
    for (int i = 0; i < verts.rows(); i++)
    {
        ss << "v " << verts(i,0) << " " << verts(i,1) << " " << verts(i,2) << "\n";
    }

    for (int i = 0; i < faces.rows(); i++)
    {
        ss << "f " << faces(i,0)+1 << " " << faces(i,1)+1 << " " << faces(i,2)+1 << "\n";
    }

    ss.close();



}

void MeshUtils::createBeamObj(const std::string& filename, const double length, const double width, const double height, const int num_subdivisions)
{
    // try and discretize so that the elements are roughly cube
    const double elem_size = std::min(width, height) / num_subdivisions;
    const int w = static_cast<int>(width/elem_size);
    const int h = static_cast<int>(height/elem_size);
    const int l = static_cast<int>(length/elem_size);
    std::cout << "size in elements: " << h << "x" << w << "x" << l << std::endl;
    
    auto index = [w, h] (int li, int wi, int hi)
    {
        return hi + wi*(h+1) + li*(w+1)*(h+1);
    };

    Eigen::Matrix<double, -1, 3> verts((h+1)*(w+1)*(l+1), 3);
    for (int li = 0; li < l+1; li++) 
    {
        for (int wi = 0; wi < w+1; wi++)
        {
            for (int hi = 0; hi < h+1; hi++)
            {
                int ind = index(li, wi, hi);
                Eigen::Vector3d vert({wi*elem_size, li*elem_size, hi*elem_size});
                // std::cout << vert.transpose() << std::endl;
                std::cout << ind << std::endl;
                verts.row(ind) = vert;
            }
        }
    }

    Eigen::Matrix<int, -1, 3> faces(4*w*h + 4*l*h + 4*l*w, 3);
    int face_ind = 0;
    // w*h faces
    for (int wi = 0; wi < w; wi++)
    {
        for (int hi = 0; hi < h; hi++)
        {
            int ff1 = index(0, wi, hi);
            int ff2 = index(0, wi+1, hi);
            int ff3 = index(0, wi+1, hi+1);
            int ff4 = index(0, wi, hi+1);

            Eigen::Vector3i front_face1({ff1, ff2, ff3});
            Eigen::Vector3i front_face2({ff1, ff3, ff4});
            faces.row(face_ind) = front_face1;
            faces.row(face_ind+1) = front_face2;

            int bf1 = index(l, wi, hi);
            int bf2 = index(l, wi+1, hi);
            int bf3 = index(l, wi+1, hi+1);
            int bf4 = index(l, wi, hi+1);
            Eigen::Vector3i back_face1({bf1, bf2, bf3});
            Eigen::Vector3i back_face2({bf1, bf3, bf4});
            faces.row(face_ind+2) = back_face1;
            faces.row(face_ind+3) = back_face2;

            face_ind += 4;
        }
    }

    // l*h faces
    for (int hi = 0; hi < h; hi++)
    {
        for (int li = 0; li < l; li++)
        {
            int rf1 = index(li, w, hi);
            int rf2 = index(li+1, w, hi);
            int rf3 = index(li+1, w, hi+1);
            int rf4 = index(li, w, hi+1);
            Eigen::Vector3i right_face1({rf1, rf2, rf3});
            Eigen::Vector3i right_face2({rf1, rf3, rf4});
            faces.row(face_ind) = right_face1;
            faces.row(face_ind+1) = right_face2;

            int lf1 = index(li, 0, hi);
            int lf2 = index(li+1, 0, hi);
            int lf3 = index(li+1, 0, hi+1);
            int lf4 = index(li, 0, hi+1);
            Eigen::Vector3i left_face1({lf1, lf2, lf3});
            Eigen::Vector3i left_face2({lf1, lf3, lf4});
            faces.row(face_ind+2) = left_face1;
            faces.row(face_ind+3) = left_face2;

            face_ind += 4;
        }
    }

    // l*w faces
    for (int li = 0; li < l; li++)
    {
        for (int wi = 0; wi < w; wi++)
        {
            int tf1 = index(li, wi, h);
            int tf2 = index(li, wi+1, h);
            int tf3 = index(li+1, wi+1, h);
            int tf4 = index(li+1, wi, h);
            Eigen::Vector3i top_face1({tf1, tf2, tf3});
            Eigen::Vector3i top_face2({tf1, tf3, tf4});
            faces.row(face_ind) = top_face1;
            faces.row(face_ind+1) = top_face2;

            int bf1 = index(li, wi, 0);
            int bf2 = index(li, wi+1, 0);
            int bf3 = index(li+1, wi+1, 0);
            int bf4 = index(li+1, wi, 0);
            Eigen::Vector3i bottom_face1({bf1, bf2, bf3});
            Eigen::Vector3i bottom_face2({bf1, bf3, bf4});
            faces.row(face_ind+2) = bottom_face1;
            faces.row(face_ind+3) = bottom_face2;

            face_ind += 4;
        }
    }

    std::ofstream ss(filename);
    for (int i = 0; i < verts.rows(); i++)
    {
        ss << "v " << verts(i,0) << " " << verts(i,1) << " " << verts(i,2) << "\n";
    }

    for (int i = 0; i < faces.rows(); i++)
    {
        ss << "f " << faces(i,0)+1 << " " << faces(i,1)+1 << " " << faces(i,2)+1 << "\n";
    }

    ss.close();



}

void MeshUtils::createTissueBlock(const std::string& filename, const double length, const double width, const double height, const int num_low_res_subdivisions, const int high_res_multiplier)
{
    // try and discretize so that the elements are roughly cube
    const double elem_size = std::min(std::min(length, width), height) / num_low_res_subdivisions;
    const double high_res_elem_size = elem_size / high_res_multiplier;
    const int w = static_cast<int>(width/elem_size);
    const int h = static_cast<int>(height/elem_size);
    const int l = static_cast<int>(length/elem_size);
    std::cout << "size in elements: " << h << "x" << w << "x" << l << std::endl;
    
    auto index = [w, h] (int li, int wi, int hi)
    {
        return hi + wi*(h+1) + li*(w+1)*(h+1);
    };

    std::vector<Eigen::Vector3d> verts((h+1)*(w+1)*(l+1));
    std::vector<Eigen::Vector3d> high_res_verts;
    for (int li = 0; li < l+1; li++) 
    {
        for (int wi = 0; wi < w+1; wi++)
        {
            for (int hi = 0; hi < h+1; hi++)
            {
                int ind = index(li, wi, hi);
                Eigen::Vector3d vert({wi*elem_size, li*elem_size, hi*elem_size});
                verts.at(ind) = vert;
            }
        }
    }

    std::vector<Eigen::Vector3i> faces;
    int face_ind = 0;
    // w*h faces
    for (int wi = 0; wi < w; wi++)
    {
        for (int hi = 0; hi < h; hi++)
        {
            int ff1 = index(0, wi, hi);
            int ff2 = index(0, wi+1, hi);
            int ff3 = index(0, wi+1, hi+1);
            int ff4 = index(0, wi, hi+1);

            Eigen::Vector3i front_face1({ff1, ff2, ff3});
            Eigen::Vector3i front_face2({ff1, ff3, ff4});
            faces.push_back(front_face1);
            faces.push_back(front_face2);

            int bf1 = index(l, wi, hi);
            int bf2 = index(l, wi+1, hi);
            int bf3 = index(l, wi+1, hi+1);
            int bf4 = index(l, wi, hi+1);
            Eigen::Vector3i back_face1({bf1, bf2, bf3});
            Eigen::Vector3i back_face2({bf1, bf3, bf4});
            faces.push_back(back_face1);
            faces.push_back(back_face2);

            face_ind += 4;
        }
    }

    // l*h faces
    for (int hi = 0; hi < h; hi++)
    {
        for (int li = 0; li < l; li++)
        {
            int rf1 = index(li, w, hi);
            int rf2 = index(li+1, w, hi);
            int rf3 = index(li+1, w, hi+1);
            int rf4 = index(li, w, hi+1);
            Eigen::Vector3i right_face1({rf1, rf2, rf3});
            Eigen::Vector3i right_face2({rf1, rf3, rf4});
            faces.push_back(right_face1);
            faces.push_back(right_face2);

            int lf1 = index(li, 0, hi);
            int lf2 = index(li+1, 0, hi);
            int lf3 = index(li+1, 0, hi+1);
            int lf4 = index(li, 0, hi+1);
            Eigen::Vector3i left_face1({lf1, lf2, lf3});
            Eigen::Vector3i left_face2({lf1, lf3, lf4});
            faces.push_back(left_face1);
            faces.push_back(left_face2);

            face_ind += 4;
        }
    }

    int center_max_w = w/2;
    int center_min_w = w/2;
    int center_max_l = l/2;
    int center_min_l = l/2;

    // l*w faces
    for (int li = 0; li < l; li++)
    {
        for (int wi = 0; wi < w; wi++)
        {
            //if (li >= li/4 && li <= 3*li/4 && wi >= wi/4 && wi <= 3*wi/4)
            if (li >= center_min_l && li <= center_max_l && wi >= center_min_w && wi <= center_max_w)
            {
                for (int i = 0; i < high_res_multiplier+1; i++)
                {
                    for (int j = 0; j < high_res_multiplier+1; j++)
                    {
                        Eigen::Vector3d vert({wi*elem_size + j*high_res_elem_size, li*elem_size + i*high_res_elem_size, height});
                        high_res_verts.push_back(vert);
                    }
                }

                int v00 = verts.size() + high_res_verts.size() - (high_res_multiplier+1) * (high_res_multiplier+1);
                for (int i = 0; i < high_res_multiplier; i++)
                {
                    for (int j = 0; j < high_res_multiplier; j++)
                    {
                        int v1 = v00 + j + i*(high_res_multiplier+1);
                        int v2 = v00 + j + 1 + i*(high_res_multiplier+1);
                        int v3 = v00 + j + 1 + (i+1)*(high_res_multiplier+1);
                        int v4 = v00 + j + (i+1)*(high_res_multiplier+1);
                        Eigen::Vector3i face1(v1, v2, v3);
                        Eigen::Vector3i face2(v1, v3, v4);
                        faces.push_back(face1);
                        faces.push_back(face2);
                        std::cout << "Face1: " << v1 << ", " << v2 << ", " << v3 << std::endl;
                        std::cout << "Face2: " << v1 << ", " << v3 << ", " << v4 << std::endl;
                    }
                }
            }
            else if (wi == center_min_w-1 && li >= center_min_l && li <= center_max_l)
            {
                for (int i = 0; i < high_res_multiplier+1; i++)
                {
                    Eigen::Vector3d vert({(wi+1)*elem_size, li*elem_size + i*high_res_elem_size, height});
                    high_res_verts.push_back(vert);
                }
                int v00 = verts.size() + high_res_verts.size() - (high_res_multiplier+1);
                // for (int i = 0; i < high_res_multiplier; i++)
                // {
                //     int v1 = v00 + i;
                //     int v2 = v00 + i + 1;
                //     if (i >= high_res_multiplier/2)
                //     {
                //         int v3 = index(wi, li+1, h);
                //         Eigen::Vector3i face({v1, v2, v3});
                //         faces.push_back(face);
                //     }
                //     else
                //     {
                //         int v3 = index(wi, li, h);
                //         Eigen::Vector3i face({v1, v2, v3});
                //         faces.push_back(face);
                //     }
                // }

                int v1_mid = index(wi, li, h);
                int v2_mid = v00 + high_res_multiplier/2;
                int v3_mid = index(wi, li+1, h);
                Eigen::Vector3i mid_face({v1_mid, v2_mid, v3_mid});
                faces.push_back(mid_face);
            }
            else
            {

                int tf1 = index(li, wi, h);
                int tf2 = index(li, wi+1, h);
                int tf3 = index(li+1, wi+1, h);
                int tf4 = index(li+1, wi, h);
                Eigen::Vector3i top_face1({tf1, tf2, tf3});
                Eigen::Vector3i top_face2({tf1, tf3, tf4});
                faces.push_back(top_face1);
                faces.push_back(top_face2);
                face_ind += 4;
            }

            int bf1 = index(li, wi, 0);
            int bf2 = index(li, wi+1, 0);
            int bf3 = index(li+1, wi+1, 0);
            int bf4 = index(li+1, wi, 0);
            Eigen::Vector3i bottom_face1({bf1, bf2, bf3});
            Eigen::Vector3i bottom_face2({bf1, bf3, bf4});
            faces.push_back(bottom_face1);
            faces.push_back(bottom_face2);

        }
    }
    
    verts.insert(verts.end(), high_res_verts.begin(), high_res_verts.end());

    std::ofstream ss(filename);
    for (int i = 0; i < verts.size(); i++)
    {
        ss << "v " << verts[i](0) << " " << verts[i](1) << " " << verts[i](2) << "\n";
    }

    for (int i = 0; i < faces.size(); i++)
    {
        ss << "f " << faces[i](0)+1 << " " << faces[i](1)+1 << " " << faces[i](2)+1 << "\n";
    }

    ss.close();
}

void MeshUtils::convertToSTL(const std::string& filename)
{
    std::cout << "MeshUtils::convertToSTL - converting " << filename << " to .stl format..." << std::endl;
    std::filesystem::path file_path(filename);

    Assimp::Importer importer;

    // And have it read the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll
    // probably to request more postprocessing than we do in this example.
    const aiScene* scene = importer.ReadFile( filename,
        aiProcess_Triangulate            |
        aiProcess_JoinIdenticalVertices  |
        aiProcess_FixInfacingNormals |
        aiProcess_SortByPType);

    // If the import failed, report it
    if (scene == nullptr)
    {
        std::cerr << "\tAssimp::Importer could not open " << filename << std::endl;
        std::cerr << "\tEnsure that the file is in a format that assimp can handle." << std::endl;
        return;
    }

    std::cout << "\tAssimp import successful" << std::endl;

    Assimp::Exporter exporter;
    const std::string& stl_filename = file_path.replace_extension(".stl").string();
    exporter.Export(scene, "stl", stl_filename);

    std::cout << "\tAssimp export successful - new file is " << stl_filename << "\n" << std::endl;
}

void MeshUtils::convertSTLtoMSH(const std::string& filename)
{
    std::cout << "MeshUtils::convertSTLtoMSH - converting " << filename << " from .stl to .msh format..." << std::endl;

    // ensure the file exists
    if (!std::filesystem::exists(filename))
    {
        std::cerr << "\t" << filename << " does not exist!" << std::endl;
        return;
    }


    std::filesystem::path file_path(filename);

    // ensure the file is an STL file
    if (file_path.extension() != ".stl" && file_path.extension() != ".STL")
    {
        std::cerr << "\t" << filename << " is not a .stl file!" << std::endl;
        return;
    }

    // open the STL file with GMSH
    gmsh::open(filename);


    int surface_loop_tag = gmsh::model::geo::addSurfaceLoop(std::vector<int>{1});

    int volume_tag = gmsh::model::geo::addVolume(std::vector<int>{surface_loop_tag});

    gmsh::model::geo::synchronize();
    gmsh::model::mesh::generate(3);

    const std::string& msh_filename = file_path.replace_extension(".msh").string();
    gmsh::write(msh_filename);

    std::cout << "\tGMSH conversion to .msh successful - new file is " << msh_filename << "\n" << std::endl;
    
}

void MeshUtils::loadMeshDataFromGmshFile(const std::string& filename, Eigen::Matrix<double, -1, 3>& verts, Eigen::Matrix<unsigned, -1, 4>& elems)
{
    std::cout << "MeshUtils::loadMeshDataFromGmshFile - loading mesh data from " << filename << " as a MeshObject..." << std::endl;

    // ensure the file exists
    if (!std::filesystem::exists(filename))
    {
        std::cerr << "\t" << filename << " does not exist!" << std::endl;
        return;
    }


    std::filesystem::path file_path(filename);

    // ensure the file is a .msh file
    if (file_path.extension() != ".msh" && file_path.extension() != ".MSH")
    {
        std::cerr << "\t" << filename << " is not a .msh file!" << std::endl;
        return;
    }

    gmsh::open(filename);

    // Get all the elementary entities in the model, as a vector of (dimension,
    // tag) pairs:
    std::vector<std::pair<int, int> > entities;
    gmsh::model::getEntities(entities);

    for(auto e : entities) {
        // Dimension and tag of the entity:
        int dim = e.first, tag = e.second;

        // Mesh data is made of `elements' (points, lines, triangles, ...), defined
        // by an ordered list of their `nodes'. Elements and nodes are identified by
        // `tags' as well (strictly positive identification numbers), and are stored
        // ("classified") in the model entity they discretize. Tags for elements and
        // nodes are globally unique (and not only per dimension, like entities).

        // A model entity of dimension 0 (a geometrical point) will contain a mesh
        // element of type point, as well as a mesh node. A model curve will contain
        // line elements as well as its interior nodes, while its boundary nodes
        // will be stored in the bounding model points. A model surface will contain
        // triangular and/or quadrangular elements and all the nodes not classified
        // on its boundary or on its embedded entities. A model volume will contain
        // tetrahedra, hexahedra, etc. and all the nodes not classified on its
        // boundary or on its embedded entities.

        // Get the mesh nodes for the entity (dim, tag):
        std::vector<std::size_t> nodeTags;
        std::vector<double> nodeCoords, nodeParams;
        gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams, dim, tag);

        unsigned vert_offset = verts.rows();
        verts.conservativeResize(vert_offset + nodeTags.size(), 3);
        for (unsigned i = 0; i < nodeTags.size(); i++)
        {
            verts(vert_offset + i, 0) = nodeCoords[i*3];
            verts(vert_offset + i, 1) = nodeCoords[i*3 + 1];
            verts(vert_offset + i, 2) = nodeCoords[i*3 + 2];
        }

        // Get the mesh elements for the entity (dim, tag):
        std::vector<int> elemTypes;
        std::vector<std::vector<std::size_t> > elemTags, elemNodeTags;
        gmsh::model::mesh::getElements(elemTypes, elemTags, elemNodeTags, dim, tag);

        // Extract all tetrahedra into a flat vector
        std::vector<unsigned> tetrahedra_vertex_indices;
        for (unsigned i = 0; i < elemTypes.size(); i++)
        {
            if (elemTypes[i] == 4)
            {
                tetrahedra_vertex_indices.insert(tetrahedra_vertex_indices.end(), elemNodeTags[i].begin(), elemNodeTags[i].end());
            }
        }

        unsigned elem_offset = elems.rows();
        unsigned num_tetrahedra = tetrahedra_vertex_indices.size()/4;
        elems.conservativeResize(elem_offset + num_tetrahedra, 4);
        for (unsigned i = 0; i < num_tetrahedra; i++)
        {
            elems(elem_offset + i, 0) = tetrahedra_vertex_indices[i*4] - 1;
            elems(elem_offset + i, 1) = tetrahedra_vertex_indices[i*4 + 1] - 1;
            elems(elem_offset + i, 2) = tetrahedra_vertex_indices[i*4 + 2] - 1;
            elems(elem_offset + i, 3) = tetrahedra_vertex_indices[i*4 + 3] - 1;
        }
    }

}