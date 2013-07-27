describe "VFS" do
  before(:each) do
    `rm -f archive.structure archive.store`
  end

  describe "Creating a new archive" do
    describe "under normal conditions" do
      before(:each) do
        `./vfs archive create 2 4`
      end

      it "should succeed" do
        expect($?.exitstatus).to eq 0
      end

      it "should create a .store file" do
        expect(File.exists? "archive.store").to be_true
      end

      it "should create a .structure file" do
        expect(File.exists? "archive.structure").to be_true
      end

      it "should put BLOCKSIZE*BLOCKCOUNT bytes in the structure file" do
        expect(File.size "archive.store").to eq 8
      end
    end

    describe "when the archive already exists" do
      before(:each) do
        `./vfs archive create 2 4`
        `./vfs archive create 2 4`
      end

      it "should exit with code 3" do
        expect($?.exitstatus).to eq 3
      end
    end
  end

  describe "Reading and writing" do
    describe "writing files" do
      it "should exit with code 2 when the vfs is not readable" do
        `./vfs archive add vfs.c vfs.c`

        expect($?.exitstatus).to eq 2
      end

      describe "when writing to a valid archive" do
        before(:each) do
          `./vfs archive create 4096 4`
        end

        it "should exit with code 0 when the file has been added" do
          `./vfs archive add vfs.c vfs.c`

          expect($?.exitstatus).to eq 0
        end

        it "should exit with code 13 when the source file does not exist" do
          `./vfs archive add ./tmp/does_not_exist hi`

          expect($?.exitstatus).to eq 13
        end
         
        it "should exit with code 11 when a file with the same name already exists" do
          `./vfs archive add vfs.c vfs.c`
          `./vfs archive add vfs.c vfs.c`

          expect($?.exitstatus).to eq 11
        end
      end
    end
  end
end
