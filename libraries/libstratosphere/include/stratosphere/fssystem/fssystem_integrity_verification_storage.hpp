/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/fs_substorage.hpp>
#include <stratosphere/fs/fs_storage_type.hpp>
#include <stratosphere/fssystem/fssystem_block_cache_buffered_storage.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 14.3.0.0 */

    class IntegrityVerificationStorage : public ::ams::fs::IStorage {
        NON_COPYABLE(IntegrityVerificationStorage);
        NON_MOVEABLE(IntegrityVerificationStorage);
        public:
            static constexpr s64 HashSize     = crypto::Sha256Generator::HashSize;

            struct BlockHash {
                u8 hash[HashSize];
            };
            static_assert(util::is_pod<BlockHash>::value);
        private:
            fs::SubStorage m_hash_storage;
            fs::SubStorage m_data_storage;
            s64 m_verification_block_size;
            s64 m_verification_block_order;
            s64 m_upper_layer_verification_block_size;
            s64 m_upper_layer_verification_block_order;
            fs::IBufferManager *m_buffer_manager;
            util::optional<fs::HashSalt> m_salt;
            bool m_is_real_data;
            fssystem::IHash256GeneratorFactory *m_hash_generator_factory;
            bool m_is_writable;
            bool m_allow_cleared_blocks;
        public:
            IntegrityVerificationStorage() : m_verification_block_size(0), m_verification_block_order(0), m_upper_layer_verification_block_size(0), m_upper_layer_verification_block_order(0), m_buffer_manager(nullptr), m_salt(util::nullopt) { /* ... */ }
            virtual ~IntegrityVerificationStorage() override { this->Finalize(); }

            Result Initialize(fs::SubStorage hs, fs::SubStorage ds, s64 verif_block_size, s64 upper_layer_verif_block_size, fs::IBufferManager *bm, fssystem::IHash256GeneratorFactory *hgf, const util::optional<fs::HashSalt> &salt, bool is_real_data, bool is_writable, bool allow_cleared_blocks);
            void Finalize();

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;

            virtual Result SetSize(s64 size) override { AMS_UNUSED(size); R_THROW(fs::ResultUnsupportedSetSizeForIntegrityVerificationStorage()); }
            virtual Result GetSize(s64 *out) override;

            virtual Result Flush() override;

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
            using IStorage::OperateRange;

            void CalcBlockHash(BlockHash *out, const void *buffer, size_t block_size) const {
                auto generator = m_hash_generator_factory->Create();
                return this->CalcBlockHash(out, buffer, block_size, generator);
            }

            s64 GetBlockSize() const {
                return m_verification_block_size;
            }
        private:
            Result ReadBlockSignature(void *dst, size_t dst_size, s64 offset, size_t size);
            Result WriteBlockSignature(const void *src, size_t src_size, s64 offset, size_t size);
            Result VerifyHash(const void *buf, BlockHash *hash, std::unique_ptr<fssystem::IHash256Generator> &generator);

            void CalcBlockHash(BlockHash *out, const void *buffer, size_t block_size, std::unique_ptr<fssystem::IHash256Generator> &generator) const;

            void CalcBlockHash(BlockHash *out, const void *buffer, std::unique_ptr<fssystem::IHash256Generator> &generator) const {
                return this->CalcBlockHash(out, buffer, static_cast<size_t>(m_verification_block_size), generator);
            }

            Result IsCleared(bool *is_cleared, const BlockHash &hash);
        private:
            static void SetValidationBit(BlockHash *hash) {
                AMS_ASSERT(hash != nullptr);
                hash->hash[HashSize - 1] |= 0x80;
            }

            static bool IsValidationBit(const BlockHash *hash) {
                AMS_ASSERT(hash != nullptr);
                return (hash->hash[HashSize - 1] & 0x80) != 0;
            }
    };

}
