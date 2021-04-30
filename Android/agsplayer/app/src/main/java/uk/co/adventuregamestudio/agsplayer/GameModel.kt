package uk.co.adventuregamestudio.agsplayer

class GameModel(name: String?, filename: String?, directory: String?, loadLastSave: Boolean?) {
   private var name: String
   private var filename: String
   private var directory: String
   private var loadLastSave: Boolean
   init {
      this.name = name!!
      this.filename = filename!!
      this.directory = directory!!
      this.loadLastSave = loadLastSave!!
   }
   fun getName(): String? {
      return name
   }
   fun setName(name: String?) {
      this.name = name!!
   }
   fun getFilename(): String? {
      return filename
   }
   fun setFilename(filename: String?) {
      this.filename = filename!!
   }
   fun getDirectory(): String? {
      return directory
   }
   fun setDirectory(directory: String?) {
      this.directory = directory!!
   }
   fun getLoadLastSave(): Boolean? {
      return loadLastSave
   }
   fun setLoadLastSave(loadLastSave: Boolean?) {
      this.loadLastSave = loadLastSave!!
   }
}
