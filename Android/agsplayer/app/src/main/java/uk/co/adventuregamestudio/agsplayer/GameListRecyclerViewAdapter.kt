package uk.co.adventuregamestudio.agsplayer

import android.view.ContextMenu
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.annotation.NonNull
import androidx.recyclerview.widget.RecyclerView

internal class GameListRecyclerViewAdapter(private var gamesList: List<GameModel>) :
    RecyclerView.Adapter<GameListRecyclerViewAdapter.GameListViewHolder>() {
    private var mClickListener: ItemClickListener? = null
    private var mCreateContextMenuListener: ItemCreateContextMenuListener? = null

    // stores and recycles views as they are scrolled off screen
    internal inner class GameListViewHolder(view: View) : RecyclerView.ViewHolder(view),
        View.OnClickListener, View.OnCreateContextMenuListener  {
        var name: TextView = view.findViewById(R.id.tvGameName)
        var filename: TextView = view.findViewById(R.id.tvGameFileName)
        var game_options: ImageView = view.findViewById(R.id.ivGameOptions)
        override fun onClick(view: View) {
            mClickListener?.onItemClick(view, adapterPosition)
        }

        init {
            itemView.setOnClickListener(this)
            itemView.setOnCreateContextMenuListener(this)
            game_options.setOnClickListener {
                // clicked on the item own three dots button
                itemView.showContextMenu();
            }
        }

        override fun onCreateContextMenu(
            menu: ContextMenu?,
            view: View?,
            menuInfo: ContextMenu.ContextMenuInfo?
        ) {
            val position = getAdapterPosition()
            mCreateContextMenuListener?.onItemCreateContextMenu(menu,view,menuInfo, position)
        }


    }

    @NonNull
    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): GameListViewHolder {
        val itemView = LayoutInflater.from(parent.context)
            .inflate(R.layout.recyclerview_row, parent, false)
        return GameListViewHolder(itemView)
    }

    override fun onBindViewHolder(holder: GameListViewHolder, position: Int) {
        val game = gamesList[position]
        holder.name.text = game.getName()
        holder.filename.text = game.getFilename()
    }

    override fun getItemCount(): Int {
        return gamesList.size
    }

    // convenience method for getting data at click position
    fun getItem(index: Int): GameModel {
        return  gamesList[index]
    }

    // allows clicks events to be caught
    fun setClickListener(itemClickListener: ItemClickListener) {
        this.mClickListener = itemClickListener
    }

    fun setCreateContextMenuListener(itemCreateContextMenuListener: ItemCreateContextMenuListener) {
        this.mCreateContextMenuListener = itemCreateContextMenuListener
    }

    // parent activity will implement this method to respond to click events
    interface ItemClickListener {
        fun onItemClick(view: View?, position: Int)
    }

    // parent activity will implement this method to respond to click events
    interface ItemCreateContextMenuListener {
        fun onItemCreateContextMenu(menu: ContextMenu?, view: View?, menuInfo: ContextMenu.ContextMenuInfo?, position: Int)
    }
}