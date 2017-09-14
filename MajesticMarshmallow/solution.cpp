#include <algorithm>
#include <iostream>
#include <array>
#include <vector>

/* stuff for mmap */
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

template <typename T>
void print_vector(const std::vector<T> &vec) {
    std::cout << '{';
    for (size_t i = 0, l = vec.size(); i < l; ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << vec[i];
    }
    std::cout << '}' << std::endl;
}

static std::vector<uint32_t> radix_sort(const std::vector<uint32_t> &numbers) {
    /* TODO: Maybe sort in place? */

    const size_t numbers_size = numbers.size();
    std::vector<uint32_t> current = numbers;

    for (size_t lsd = 0; lsd < 32; ++lsd) {
        std::array<std::vector<uint32_t>, 2> buckets;

        for (size_t i = 0; i < numbers_size; ++i) {
            const uint32_t number = current[i];
            const uint32_t mask = 1 << lsd;

            buckets[(number & mask) == mask].push_back(number);
        }

        if (buckets[0].size() == numbers_size) {
            return current;
        }

        current.clear();
        for (size_t i = 0; i < 2; ++i) {
            const std::vector<uint32_t> &bucket = buckets[i];
            current.insert(current.end(), bucket.begin(), bucket.end());
        }
    }
    throw std::logic_error("How could this happen?");
}

int main(void) {
    /* Faster IO */
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);

    int fd = open("numbers.bin", O_RDONLY);
    if (fd == -1) {
        puts("Could not open numbers.bin");
        return EXIT_FAILURE;
    }

    /* I am expecting all of these calls to work correctly due to being able to
     * create the file descriptor. */
    struct stat st;
    stat("numbers.bin", &st);
    size_t integer_amount = st.st_size / sizeof(int32_t);
    int32_t *integers = (int32_t*) mmap(NULL, integer_amount, PROT_READ, MAP_SHARED, fd, 0);
    if (integers == MAP_FAILED) {
        puts("Could not create the read mmap.");
        return EXIT_FAILURE;
    }

    std::vector<uint32_t> positive_numbers;
    std::vector<uint32_t> negative_numbers;
    for (size_t i = 0; i < integer_amount; ++i) {
        int32_t number = integers[i];
        if (number < 0) {
            negative_numbers.push_back(-number);
        } else {
            positive_numbers.push_back(number);
        }
    }

    close(fd);

    /* I'm not sure if reusing the same variables is wise here. */
    fd = open("sorted_numbers.bin", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        puts("Could not open sorted_numbers.bin");
        return EXIT_FAILURE;
    }

    lseek(fd, integer_amount * sizeof(int32_t) - 1, SEEK_SET);
    write(fd, "", 1);
    lseek(fd, 0, SEEK_SET);

    integers = (int32_t*) mmap(NULL, integer_amount, PROT_WRITE, MAP_SHARED, fd, 0);
    if (integers == MAP_FAILED) {
        puts("Could not create the write mmap.");
        return EXIT_FAILURE;
    }

    negative_numbers = radix_sort(negative_numbers);
    for (auto rit = negative_numbers.rbegin(), rend = negative_numbers.rend(); rit != rend; ++rit) {
        int32_t v = *rit;
        *(integers++) = -v;
    }

    for (uint32_t v : radix_sort(positive_numbers)) {
        *(integers++) = v;
    }

    close(fd);
}