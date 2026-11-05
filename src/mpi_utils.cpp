#include "mpi_utils.hpp"
#include <algorithm>

Domain1D compute_domain_1d(int total_height, int width, int rank, int size,
                           int halo_radius, int channels_per_pixel) {
    Domain1D d;
    d.total_height = total_height;
    d.width = width;
    d.rank = rank;
    d.size = size;
    d.halo_radius = halo_radius;

    d.sendcounts.resize(size);
    d.displs.resize(size);
    d.row_counts.resize(size);
    d.row_starts.resize(size);

    int base_rows = total_height / size;
    int remainder = total_height % size;

    for (int r = 0; r < size; ++r) {
        int r_rows = base_rows + (r < remainder ? 1 : 0);
        int r_start = r * base_rows + std::min(r, remainder);

        d.row_counts[r] = r_rows;
        d.row_starts[r] = r_start;
        d.sendcounts[r] = r_rows * width * channels_per_pixel;
        d.displs[r] = r_start * width * channels_per_pixel;
    }

    d.local_height = d.row_counts[rank];
    d.start_row = d.row_starts[rank];
    d.end_row = d.start_row + d.local_height;

    return d;
}
