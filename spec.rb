describe "VFS" do
  before(:each) do
    `rm -rf tmp`
    `mkdir tmp`
  end

  describe "Creating a new archive" do
    describe "under normal conditions" do
      before(:each) do
        `./vfs ./tmp/archive create 2 4`
      end

      it "should succeed" do
        expect($?.exitstatus).to eq 0
      end

      it "should create a .store file" do
        expect(File.exists? "./tmp/archive.store").to be_true
      end

      it "should create a .structure file" do
        expect(File.exists? "./tmp/archive.structure").to be_true
      end

      it "should put BLOCKSIZE*BLOCKCOUNT bytes in the structure file" do
        expect(File.size "./tmp/archive.store").to eq 8
      end
    end

    describe "when the archive already exists" do
      before(:each) do
        `./vfs ./tmp/archive create 2 4`
        `./vfs ./tmp/archive create 2 4`
      end

      it "should exit with code 3" do
        expect($?.exitstatus).to eq 3
      end
    end
  end

  describe "Reading and writing" do
    describe "writing files" do
      it "should exit with code 2 when the vfs is not readable" do
        `./vfs ./tmp/archive add vfs.c vfs.c`

        expect($?.exitstatus).to eq 2
      end

      describe "when writing to a valid archive" do
        before(:each) do
          `./vfs ./tmp/archive create 4096 4`
        end

        it "should exit with code 0 when the file has been added" do
          `./vfs ./tmp/archive add vfs.c vfs.c`

          expect($?.exitstatus).to eq 0
        end

        it "should exit with code 13 when the source file does not exist" do
          `./vfs ./tmp/archive add ./tmp/does_not_exist hi`

          expect($?.exitstatus).to eq 13
        end
         
        it "should exit with code 11 when a file with the same name already exists" do
          `./vfs ./tmp/archive add vfs.c vfs.c`
          `./vfs ./tmp/archive add vfs.c vfs.c`

          expect($?.exitstatus).to eq 11
        end
        
        it "should exit with code 12 when there is not enough space" do
          `dd count=1 bs=10M if=/dev/zero of=./tmp/big_file 2>&1 > /dev/null`
          `./vfs ./tmp/archive add ./tmp/big_file too_big`

          expect($?.exitstatus).to eq 12
        end
      end
    end
  end
end
