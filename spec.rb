describe "VFS" do
  before(:each) do
    `rm archive.structure archive.store`
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
end
